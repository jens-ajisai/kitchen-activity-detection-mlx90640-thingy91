/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE commCtrl_module

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

#ifdef CONFIG_CJSON_LIB
#include "cJSON.h"
#endif

#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <math.h>
#include <memfault/core/trace_event.h>
#include <stdio.h>
#include <sys/base64.h>
#include <zephyr.h>

#include "common/memory_hook.h"
#include "bt/bt_module_event.h"
#include "../commCtrl/commCtrl_module_event.h"
#include "sensors/sensors_module_event.h"
#include "mcu_exchange/mcu_exchange_module_event.h"
#include "common/modules_common.h"
#include "utils.h"

LOG_MODULE_REGISTER(commCtrl_module, CONFIG_COMMCTRL_MODULE_LOG_LEVEL);

struct commCtrl_msg_data {
  union {
    struct module_state_event module_state;
    struct sensors_module_event sensors;
    struct sensors_data_module_event sensors_data;
    struct bt_module_event bt;
    struct commCtrl_module_event commCtrl;
  } module;
  struct event_dyndata *dyndata;
};

/* COMMCTRL module message queue. */
#define COMMCTRL_QUEUE_ENTRY_COUNT 15
#define COMMCTRL_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_commCtrl, sizeof(struct commCtrl_msg_data), COMMCTRL_QUEUE_ENTRY_COUNT,
              COMMCTRL_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "commCtrl",
    .msg_q = &msgq_commCtrl,
};
static enum state_type { STATE_INIT, STATE_READY } state;

static char *state2str(enum state_type new_state) {
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

static bool event_handler(const struct event_header *eh) {
  struct commCtrl_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(commCtrl_module, commCtrl)
  TRANSLATE_EVENT_TO_MSG(bt_module, bt)
  TRANSLATE_EVENT_TO_MSG(sensors_module, sensors)
  TRANSLATE_EVENT_TO_MSG(sensors_data_module, sensors_data)

  if (is_module_state_event(eh)) {
    struct module_state_event *event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  if (is_sensors_data_module_event(eh)) {
    struct sensors_data_module_event *event = cast_sensors_data_module_event(eh);
    msg.dyndata = my_malloc(sizeof(struct event_dyndata) + event->dyndata.size, HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata, &event->dyndata, sizeof(struct event_dyndata) + event->dyndata.size);
  }

  ENQUEUE_MSG(commCtrl, COMMCTRL)

  return false;
}

static void init_module() { state_set(STATE_READY); }

static void on_state_init(struct commCtrl_msg_data *msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_module();
  }
}

static void sendToNrf9160(uint8_t *data, uint16_t size, enum mcu_exchange_module_event_type type) {
  if (size > 0) {
    struct mcu_exchange_module_event *event = new_mcu_exchange_module_event(size);
    event->type = type;
    event->dyndata.size = size;
    memcpy(event->dyndata.data, data, size);
    EVENT_SUBMIT(event);
  }
}

static void sendToBle(uint8_t *data, uint16_t len) {
  {
    struct bt_data_module_event* bt_event = new_bt_data_module_event(len);
    bt_event->type = BT_EVT_HEATMAP_SEND;
    bt_event->dyndata.size = len;
    memcpy(bt_event->dyndata.data, data, len);
    bt_event->useBase64 = true;
    EVENT_SUBMIT(bt_event);
  }
  {
    struct bt_data_module_event* bt_event = new_bt_data_module_event(1);
    bt_event->type = BT_EVT_HEATMAP_SEND;
    bt_event->dyndata.size = 1;
    memset(bt_event->dyndata.data, 0, 1);
    bt_event->useBase64 = false;
    EVENT_SUBMIT(bt_event);
  }
}

static void prepareAndSendHeatmap(uint8_t *data, uint16_t len, int64_t timestamp,
                                  uint8_t sensorsId) {
  uint16_t entriesCount = len / sizeof(float);
  int16_t *heatMap =
      my_malloc(entriesCount * sizeof(uint16_t), HEAP_MEMORY_STATISTICS_ID_COMM_CTRL_HEATMAP_DATA);
  convertAndScaleHeatMap(heatMap, data, entriesCount);

  uint8_t *heatMapComp;
  uint16_t size = compressData(&heatMapComp, (int8_t *)heatMap, entriesCount * sizeof(uint16_t));
  my_free(heatMap);

  if (size > 0) {
    sendToBle(data, len);
    sendToNrf9160(heatMapComp, size, MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY);
  }
  my_free(heatMapComp);
}

static void on_ready_state(struct commCtrl_msg_data *msg) {
  if (IS_EVENT(msg, sensors_data, SENSORS_EVT_HEAT_MAP_DATA_READY)) {
    prepareAndSendHeatmap(msg->dyndata->data,
                          msg->dyndata->size,
                          msg->module.sensors_data.timestamp,
                          msg->module.sensors_data.sensorsId);

  }
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE  
   else if (IS_EVENT(msg, bt, BT_EVT_SET_HEATMAP_INTERVAL)) {
    SEND_VAL(sensors, SENSORS_EVT_SET_INTERVAL, msg->module.bt.data.val);
  }
#endif
}

static void on_all_states(struct commCtrl_msg_data *msg) {
  if (msg->dyndata) {
    my_free(msg->dyndata);
  }
}

static void module_thread_fn(void) {
  struct commCtrl_msg_data msg;

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
        on_ready_state(&msg);
        break;
      default:
        LOG_WRN("Unknown commCtrl app state");

        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(commCtrl_module_thread, CONFIG_COMMCTRL_THREAD_STACK_SIZE, module_thread_fn, NULL,
                NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_FINAL(MODULE, bt_module_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, sensors_module_event);
EVENT_SUBSCRIBE(MODULE, sensors_data_module_event);
EVENT_SUBSCRIBE(MODULE, commCtrl_module_event);

