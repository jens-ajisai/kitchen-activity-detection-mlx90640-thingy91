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

#define MODULE sensors_module

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <zephyr.h>

#include "common/memory_hook.h"
#include "common/modules_common.h"
#include "sensors/sensors_module_event.h"
#include "sensors_ids.h"

#include "util_settings.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_SENSORS_MODULE_LOG_LEVEL);

struct sensors_msg_data {
  union {
    struct module_state_event module_state;
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

static bool event_handler(const struct event_header* eh) {
  struct sensors_msg_data msg = {0};
  bool enqueue_msg = false;
  TRANSLATE_EVENT_TO_MSG(sensors_module, sensors)

  if (is_module_state_event(eh)) {
    struct module_state_event* event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  ENQUEUE_MSG(sensors, SENSORS)

  return false;
}

#include "sensor_mlx60940.h"

static void notify_heatMap_cb(float* heat_map, uint16_t size) {
  LOG_INF("%f, %f, %f, %f, %f, %f, %f, %f", heat_map[3], heat_map[19], heat_map[78], heat_map[145],
          heat_map[200], heat_map[345], heat_map[500], heat_map[720]);

  struct sensors_data_module_event* event = new_sensors_data_module_event(size);
  event->type = SENSORS_EVT_HEAT_MAP_DATA_READY;
  event->timestamp = k_uptime_get();
  event->sensorsId = SENSOR_HEAT_MAP;
  memcpy(event->dyndata.data, heat_map, size);
  event->dyndata.size = size;
  EVENT_SUBMIT(event);
}

static struct mlx60940_cb mlx60940_callbacs = {
    .notify_heatMap_cb = notify_heatMap_cb,
};

static void init_module() {
  int err = init_mlx60940(&mlx60940_callbacs);
  if (err) {
    SEND_ERROR(sensors, SENSORS_EVT_ERROR, err);
  }
}

static void on_state_init(struct sensors_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_module();
    settings_util_init();
    state_set(STATE_READY);
  }
}

static void on_state_ready(struct sensors_msg_data* msg) {
  if (IS_EVENT(msg, sensors, SENSORS_EVT_SET_INTERVAL)) {
    set_inverval_mlx60940(msg->module.sensors.data.val);
  }
}

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
