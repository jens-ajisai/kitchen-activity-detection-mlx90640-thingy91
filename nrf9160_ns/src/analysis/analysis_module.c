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

#define MODULE analysis_module

#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <memfault/core/trace_event.h>
#include <stdio.h>
#include <sys/base64.h>
#include <zephyr.h>

#include "common/memory_hook.h"
#include "../analysis/analysis_module_event.h"
#include "mqtt/mqtt_module_event.h"
#include "../sensors/sensors_module_event.h"
#include "mcu_exchange/mcu_exchange_module_event.h"
#include "common/modules_common.h"
#include "utils.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_ANALYSIS_MODULE_LOG_LEVEL);

struct analysis_msg_data {
  union {
    struct module_state_event module_state;
    struct analysis_module_event analysis;
    struct sensors_module_event sensors;
    struct mcu_exchange_module_event mcu_exchange;
  } module;
  struct event_dyndata *dyndata;
};

/* Sensor module message queue. */
#define ANALYSIS_QUEUE_ENTRY_COUNT 10
#define ANALYSIS_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_analysis, sizeof(struct analysis_msg_data), ANALYSIS_QUEUE_ENTRY_COUNT,
              ANALYSIS_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "analysis",
    .msg_q = &msgq_analysis,
};
static enum state_type {
  STATE_INIT,
  STATE_READY,
} state;

/* Convenience functions used in internal state handling. */
static char* state2str(enum state_type new_state) {
  switch (new_state) {
    case STATE_INIT:
      return "STATE_INIT";
    case STATE_READY:
      return "STATE_READY";
    default:
      return "Unknown";
  }
}

DECLARE_SET_STATE()

static bool event_handler(const struct event_header* eh) {
  struct analysis_msg_data msg = {0};
  bool enqueue_msg = false;
  TRANSLATE_EVENT_TO_MSG(analysis_module, analysis)
  TRANSLATE_EVENT_TO_MSG(sensors_module, sensors)
  TRANSLATE_EVENT_TO_MSG(module_state, module_state)
  TRANSLATE_EVENT_TO_MSG(mcu_exchange_module, mcu_exchange)

  if (is_mcu_exchange_module_event(eh)) {
    struct mcu_exchange_module_event *event = cast_mcu_exchange_module_event(eh);
    msg.dyndata = my_malloc(sizeof(struct event_dyndata) + event->dyndata.size, HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata, &event->dyndata, sizeof(struct event_dyndata) + event->dyndata.size);
  }

  LOG_DBG("event_handler enqueue_msg.");
  ENQUEUE_MSG(analysis, ANALYSIS)

  return false;
}

static void init_module() {}

static void on_state_init(struct analysis_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_module();
    state_set(STATE_READY);
  }
}

char* ValueToString(float value) {
  char* result = my_malloc(5, HEAP_MEMORY_STATISTICS_ID_ANALYSIS_VAL_STRING);
  sprintf(result, "%.2f", value);
  return result;
}

static void on_state_ready(struct analysis_msg_data* msg) {
  if (IS_EVENT(msg, sensors, SENSORS_EVT_ENV_TEMPERATURE)) {
    struct mqtt_module_event* event;
    event = new_mqtt_module_event();
    event->data.msg.topic = CONFIG_MQTT_TOPIC_TEMPERATURE;
    event->data.msg.message = ValueToString(msg->module.sensors.data.val);
    event->type = MQTT_EVT_SEND;
    EVENT_SUBMIT(event);
  } else if (IS_EVENT(msg, sensors, SENSORS_EVT_ENV_HUMIDITY)) {
    struct mqtt_module_event* event;
    event = new_mqtt_module_event();
    event->data.msg.topic = CONFIG_MQTT_TOPIC_HUMIDITY;
    event->data.msg.message = ValueToString(msg->module.sensors.data.val);
    event->type = MQTT_EVT_SEND;
    EVENT_SUBMIT(event);
  } else if (IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY)) {
    LOG_DBG("Handle MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY");
    struct mqtt_module_event* event;
    event = new_mqtt_module_event();
    event->data.msg.topic = CONFIG_MQTT_TOPIC_HEATMAP;
    event->data.msg.message = HeatmapToBase64Image(msg->dyndata->data,
                                                   msg->dyndata->size);
    event->type = MQTT_EVT_SEND;
    EVENT_SUBMIT(event);
  }
}
static void on_all_states(struct analysis_msg_data* msg) {
  LOG_DBG("on_all_states.");

  if (msg->dyndata) {
    my_free(msg->dyndata);
  }
}

static void module_thread_fn(void) {
  struct analysis_msg_data msg;

  self.thread_id = k_current_get();

  module_start(&self);
  state_set(STATE_INIT);

  while (true) {
    module_get_next_msg(&self, &msg);

    switch (state) {
      case STATE_INIT:
        on_state_init(&msg);
        break;
      case STATE_READY:
        on_state_ready(&msg);
        break;
      default:
        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(analysis_module_thread, CONFIG_ANALYSIS_THREAD_STACK_SIZE, module_thread_fn, NULL,
                NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, sensors_module_event);
EVENT_SUBSCRIBE_FINAL(MODULE, analysis_module_event);
EVENT_SUBSCRIBE(MODULE, mcu_exchange_module_event);
