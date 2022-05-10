/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "fs_module_event.h"

#include <stdio.h>

static char* get_evt_type_str(enum fs_module_event_type type) {
  switch (type) {
    case FS_EVT_ERROR:
      return "FS_EVT_ERROR";
    default:
      return "Unknown event";
  }
}

static int log_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct fs_module_event* event = cast_fs_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(fs_module_event, CONFIG_FS_EVENTS_LOG, log_event, NULL);

static char* get_data_evt_type_str(enum fs_data_module_event_type type) {
  switch (type) {
    case FS_EVT_READ_FILE_REQ:
      return "FS_EVT_READ_FILE_REQ";
    case FS_EVT_READ_FILE_RES:
      return "FS_EVT_READ_FILE_RES";
    case FS_EVT_WRITE_FILE:
      return "FS_EVT_WRITE_FILE";
    case FS_EVT_APPEND_FILE:
      return "FS_EVT_APPEND_FILE";
    case FS_EVT_DELETE_FILE:
      return "FS_EVT_DELETE_FILE";
    default:
      return "Unknown event";
  }
}

static int log_data_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct fs_data_module_event* event = cast_fs_data_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_data_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(fs_data_module_event, CONFIG_FS_EVENTS_LOG, log_data_event, NULL);