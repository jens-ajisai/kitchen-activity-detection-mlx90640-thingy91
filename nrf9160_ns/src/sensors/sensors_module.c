/*
 * The module architecture (using states, message queue in a separate thread, using events for
 * communication) is based on the Application Event Manager as used in the Asset Tracker v2
 * Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE sensors91_module

#include <caf/events/module_state_event.h>
#include <caf/events/sensor_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <zephyr.h>

#include "../sensors/sensors_module_event.h"
#include "mcu_exchange/mcu_exchange_module_event.h"
#include "common/modules_common.h"
#include "sensors_ids.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_SENSORS91_MODULE_LOG_LEVEL);

struct sensors_msg_data {
  union {
    struct module_state_event module_state;
    struct sensor_event sensor;
    struct sensor_state_event sensor_state;
    struct sensors_module_event sensors;
  } module;
};

/* Sensor module message queue. */
#define SENSORS_QUEUE_ENTRY_COUNT 10
#define SENSORS_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_sensors, sizeof(struct sensors_msg_data), SENSORS_QUEUE_ENTRY_COUNT,
              SENSORS_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "sensors",
    .msg_q = &msgq_sensors,
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

static bool handle_sensor_event(const struct sensor_event* event) {
  if (strcmp(event->descr, "accel_xyz") == 0) {
    float* data_ptr = sensor_event_get_data_ptr(event);
    LOG_DBG("%s, %d, %f, %f, %f", event->descr, sensor_event_get_data_cnt(event), data_ptr[0],
            data_ptr[1], data_ptr[2]);
  }

  else if (strcmp(event->descr, "env") == 0) {
    float* data_ptr = sensor_event_get_data_ptr(event);

    { SEND_VAL(sensors, SENSORS_EVT_ENV_TEMPERATURE, data_ptr[0]); }
    { SEND_VAL(sensors, SENSORS_EVT_ENV_HUMIDITY, data_ptr[1]); }

    if (sensor_event_get_data_cnt(event) == 4) {
      LOG_DBG("%s, %f, %f, %f, %f", event->descr, data_ptr[0], data_ptr[1], data_ptr[2],
              data_ptr[3]);
    }
  }

  else if (strcmp(event->descr, "color") == 0) {
    float* data_ptr = sensor_event_get_data_ptr(event);

    if (sensor_event_get_data_cnt(event) == 4) {
      LOG_DBG("%s, %f, %f, %f, %f", event->descr, data_ptr[0], data_ptr[1], data_ptr[2],
              data_ptr[3]);
      if ((data_ptr[0] + data_ptr[1] + data_ptr[2]) > CONFIG_SENSORS_HIGH_COLOR_INTENSITY_THRESHOLD) {
        // start advertising BLE on high color intensity
        struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(0);
        event->type = MCU_EXCHANGE_EVT_BUTTON;
        EVENT_SUBMIT(event);
      }
    }
  }

  return false;
}

static bool handle_sensor_state_event(const struct sensor_state_event* event) {
  LOG_DBG("%s, %d", event->descr, event->state);
  LOG_WRN("handle_sensor_event handle_sensor_state_event disabled.");

  return false;
}

static bool event_handler(const struct event_header* eh) {
  struct sensors_msg_data msg = {0};
  bool enqueue_msg = false;
  TRANSLATE_EVENT_TO_MSG(sensors_module, sensors)
  TRANSLATE_EVENT_TO_MSG(module_state, module_state)

  // must be handeled here as the values will be immediately cleared
  if (is_sensor_event(eh)) {
    return handle_sensor_event(cast_sensor_event(eh));
  }

  if (is_sensor_state_event(eh)) {
    return handle_sensor_state_event(cast_sensor_state_event(eh));
  }
  ENQUEUE_MSG(sensors, SENSORS)

  return false;
}

static void init_module() {}

static void on_state_init(struct sensors_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_module();
    state_set(STATE_READY);
  }
}

static void on_state_ready(struct sensors_msg_data* msg) {}

static void on_all_states(struct sensors_msg_data* msg) {}

static void module_thread_fn(void) {
  struct sensors_msg_data msg;

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

K_THREAD_DEFINE(sensors_module_thread, CONFIG_SENSORS_THREAD_STACK_SIZE, module_thread_fn, NULL,
                NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_FINAL(MODULE, sensors_module_event);
EVENT_SUBSCRIBE(MODULE, sensor_event);
EVENT_SUBSCRIBE(MODULE, sensor_state_event);
