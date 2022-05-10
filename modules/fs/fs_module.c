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

#include <event_manager.h>
#include <zephyr.h>

#define MODULE fs_module

#include <caf/events/module_state_event.h>
#include <logging/log.h>
#include <stdio.h>

#include "common/memory_hook.h"
#include "common/modules_common.h"
#include "fs/fs_module_event.h"
#include "fs_impl.h"

#include "utils.h"

LOG_MODULE_REGISTER(fs_module, CONFIG_FS_MODULE_LOG_LEVEL);

struct fs_msg_data {
  union {
    struct fs_module_event fs;
    struct fs_data_module_event fs_data;
    struct module_state_event module_state;
  } module;
  struct event_dyndata *dyndata;  
};

/* Diag module message queue. */
#define FS_QUEUE_ENTRY_COUNT 10
#define FS_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_fs, sizeof(struct fs_msg_data), FS_QUEUE_ENTRY_COUNT, FS_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "fs",
    .msg_q = &msgq_fs,
};
static enum state_type { STATE_INIT, STATE_READY, STATE_ERROR } state;

static char* state2str(enum state_type new_state) {
  switch (new_state) {
    case STATE_INIT:
      return "STATE_INIT";
    case STATE_READY:
      return "STATE_READY";
    case STATE_ERROR:
      return "STATE_ERROR";
    default:
      return "Unknown";
  }
}

DECLARE_SET_STATE()

static bool event_handler(const struct event_header* eh) {
  struct fs_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(fs_module, fs)
  TRANSLATE_EVENT_TO_MSG(fs_data_module, fs_data)

  if (is_module_state_event(eh)) {
    struct module_state_event* event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  if (is_fs_data_module_event(eh)) {
    struct fs_data_module_event *event = cast_fs_data_module_event(eh);
    msg.dyndata = my_malloc(sizeof(struct event_dyndata) + event->dyndata.size, HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata, &event->dyndata, sizeof(struct event_dyndata) + event->dyndata.size);
  }  

  ENQUEUE_MSG(fs, FS)

  return false;
}

void init_fs_module() {
  LOG_INF("init_fs_module start");
  int ret = fs_module_setup();
  LOG_INF("init_fs_module, ret=%d", ret);
  if (ret) {
    SEND_ERROR(fs, FS_EVT_ERROR, ret);
    state_set(STATE_ERROR);
    blink(2000);
  } else {
    state_set(STATE_READY);
  };
}

static void on_state_init(struct fs_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_fs_module();
  }
}

static void on_state_ready(struct fs_msg_data* msg) {
  if (IS_EVENT(msg, fs_data, FS_EVT_READ_FILE_REQ)) {
    char* buf = my_malloc(CONFIG_FILE_READ_BUFFER_SIZE, HEAP_MEMORY_STATISTICS_ID_FS_READ_BUFFER);
    size_t bytes_read = CONFIG_FILE_READ_BUFFER_SIZE;

    int err = fs_helper_file_read(msg->module.fs_data.file_path, buf, &bytes_read);
    if (err) {
      SEND_ERROR(fs, FS_EVT_ERROR, err);
    }

    struct fs_data_module_event* event = new_fs_data_module_event(CONFIG_FILE_READ_BUFFER_SIZE);
    event->type = FS_EVT_READ_FILE_RES;
    memcpy(event->file_path, msg->module.fs_data.file_path, CONFIG_MAX_PATH_LENGTH);
    memcpy(event->dyndata.data, buf, bytes_read);
    my_free(buf);
    event->dyndata.size = bytes_read;
    EVENT_SUBMIT(event);
  }

  if ((IS_EVENT(msg, fs_data, FS_EVT_WRITE_FILE)) || (IS_EVENT(msg, fs_data, FS_EVT_APPEND_FILE))) {
    int err = fs_helper_file_write(
        msg->module.fs_data.file_path, msg->dyndata->data,
        msg->dyndata->size, (msg->module.fs_data.type == FS_EVT_APPEND_FILE));
    if (err) {
      SEND_ERROR(fs, FS_EVT_ERROR, err);
    }
  }

  if (IS_EVENT(msg, fs_data, FS_EVT_DELETE_FILE)) {
    int err = fs_helper_file_delete(msg->module.fs_data.file_path);
    if (err) {
      SEND_ERROR(fs, FS_EVT_ERROR, err);
    }
  }
}

static void on_all_states(struct fs_msg_data *msg) {
  if (msg->dyndata) {
    my_free(msg->dyndata);
  }
}

static void module_thread_fn(void) {
  struct fs_msg_data msg;

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
        LOG_WRN("Unknown state");
        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(fs_module_thread, CONFIG_FS_THREAD_STACK_SIZE, module_thread_fn, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, fs_module_event);
EVENT_SUBSCRIBE(MODULE, fs_data_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
