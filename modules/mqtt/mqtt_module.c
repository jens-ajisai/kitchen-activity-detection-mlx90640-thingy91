/*
 * The module architecture (using states, message queue in a separate thread, using events for communication)
 * is based on the Application Event Manager as used in the Asset Tracker v2 Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE mqtt_module
#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <memfault/core/trace_event.h>
#include <memfault/metrics/metrics.h>
#include <net/mqtt.h>
#include <zephyr.h>

#include "../adafruit_certificates.h"
#include "mqtt/mqtt_module_event.h"
#include "common/modules_common.h"
#include "mqtt_client.h"
#include "common/memory_hook.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_MQTT_MODULE_LOG_LEVEL);

struct mqtt_msg_data {
  union {
    struct module_state_event module_state;
    struct mqtt_module_event mqtt;
  } module;
};

/* Sensor module message queue. */
#define MQTT_QUEUE_ENTRY_COUNT 10
#define MQTT_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_mqtt, sizeof(struct mqtt_msg_data), MQTT_QUEUE_ENTRY_COUNT,
              MQTT_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "mqtt",
    .msg_q = &msgq_mqtt,
};

/* Sensor module super states. */
static enum state_type { STATE_INIT, STATE_CONNECTED, STATE_DISCONNECTED, STATE_ERROR } state;

/* Convenience functions used in internal state handling. */
static char* state2str(enum state_type new_state) {
  switch (new_state) {
    case STATE_INIT:
      return "STATE_INIT";
    case STATE_CONNECTED:
      return "STATE_CONNECTED";
    case STATE_DISCONNECTED:
      return "STATE_DISCONNECTED";
    case STATE_ERROR:
      return "STATE_ERROR";
    default:
      return "Unknown";
  }
}

DECLARE_SET_STATE()

/* Handlers */
static bool event_handler(const struct event_header* eh) {
  struct mqtt_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(mqtt_module, mqtt)

  if (is_module_state_event(eh)) {
    struct module_state_event* event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  ENQUEUE_MSG(mqtt, MQTT)

  return false;
}

/* The mqtt client struct */
static struct mqtt_client client;

static sec_tag_t adafruit_tls_sec_tag[] = {kAdafruitRootCert_DigicertRootCa,
                                           kAdafruitRootCert_DigicertGeotrustCa,
                                           kAdafruitRootCert_Adafruit};

static struct mqtt_client_info client_info = {
    .brokerHostname = CONFIG_MQTT_BROKER_HOSTNAME,
    .brokerPort = CONFIG_MQTT_BROKER_PORT,
    .clientId = CONFIG_MQTT_CLIENT_ID,
    .userName = CONFIG_MQTT_USER_NAME,
    .password = CONFIG_MQTT_PASSWORD,
    .libtls = CONFIG_MQTT_LIB_TLS,
    .tlsSecTags = adafruit_tls_sec_tag,
    .tlsSecTagsLen = sizeof(adafruit_tls_sec_tag)/sizeof(adafruit_tls_sec_tag[0]),
    .peerVerify = CONFIG_MQTT_TLS_PEER_VERIFY,
    .sessionCaching = CONFIG_MQTT_TLS_SESSION_CACHING,
};

static void mqtt_connected_cb(const int result) {
  state_set(STATE_CONNECTED);
  SEND_EVENT(mqtt, MQTT_EVT_CONNECTED);
}

static void mqtt_disconnected_cb(const int result) {
  state_set(STATE_DISCONNECTED);
  SEND_EVENT(mqtt, MQTT_EVT_DISCONNECTED);
}

static void mqtt_received_cb(const int result, const uint8_t* data, const size_t le) {
  if (result != 0) {
    int err = mqtt_client_module_disconnect(&client);
    if (err) {
      LOG_ERR("Could not disconnect: %d", err);
    }
  } else {
    struct mqtt_module_event* mqtt_module_event = new_mqtt_module_event();
    mqtt_module_event->type = MQTT_EVT_RECEIVED;
    EVENT_SUBMIT(mqtt_module_event);
  }
}

static struct mqtt_client_module_cb mqtt_client_module_callbacs = {
    .mqtt_connected_cb = mqtt_connected_cb,
    .mqtt_disconnected_cb = mqtt_disconnected_cb,
    .mqtt_received_cb = mqtt_received_cb,
};

static void on_state_init(struct mqtt_msg_data* msg) {
  // need to wait for lte connected to connect
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    if (mqtt_client_module_init(&client, &client_info, &mqtt_client_module_callbacs)) {
      state_set(STATE_ERROR);
    } else {
      state_set(STATE_DISCONNECTED);
      if (mqtt_client_module_connect(&client)) {
        //      state_set(STATE_ERROR);
      }
    }
  } else if ((IS_EVENT(msg, mqtt, MQTT_EVT_SUBSCRIBE)) || (IS_EVENT(msg, mqtt, MQTT_EVT_SEND))) {
    SEND_ERROR(mqtt, MQTT_EVT_ERROR, 99);
  }
}

static void on_state_connected(struct mqtt_msg_data* msg) {
  if (IS_EVENT(msg, mqtt, MQTT_EVT_SUBSCRIBE)) {
    mqtt_client_module_subscribe(&client, msg->module.mqtt.data.msg.topic);
  } else if (IS_EVENT(msg, mqtt, MQTT_EVT_SEND)) {
    mqtt_client_module_data_publish(&client, msg->module.mqtt.data.msg.topic,
                                    msg->module.mqtt.data.msg.message,
                                    strlen(msg->module.mqtt.data.msg.message) + 1);
    memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(MqttSendFrequency), 1);
    // topic is a static string
    // my_free(msg->module.mqtt.data.msg.topic);
    my_free(msg->module.mqtt.data.msg.message);
  }
}

static void on_state_disconnected(struct mqtt_msg_data* msg) {
  if (IS_EVENT(msg, mqtt, MQTT_EVT_CONNECT)) {
    if (mqtt_client_module_connect(&client)) {
      //      state_set(STATE_ERROR);
    }
  }
  if ((IS_EVENT(msg, mqtt, MQTT_EVT_SUBSCRIBE)) || (IS_EVENT(msg, mqtt, MQTT_EVT_SEND))) {
    SEND_ERROR(mqtt, MQTT_EVT_ERROR, 99);
  }
}

/* Message handler for all states. */
static void on_all_states(struct mqtt_msg_data* msg) {}

static void module_thread_fn(void) {
  struct mqtt_msg_data msg;

  self.thread_id = k_current_get();

  module_start(&self);
  state_set(STATE_INIT);

  while (true) {
    module_get_next_msg(&self, &msg);

    switch (state) {
      case STATE_INIT:
        on_state_init(&msg);
        break;
      case STATE_CONNECTED:
        on_state_connected(&msg);
        break;
      case STATE_DISCONNECTED:
        on_state_disconnected(&msg);
        break;
      case STATE_ERROR:
        break;
      default:
        LOG_WRN("Unknown mqtt module state.");

        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(mqtt_module_thread, CONFIG_MQTT_THREAD_STACK_SIZE, module_thread_fn, NULL, NULL,
                NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, mqtt_module_event);
