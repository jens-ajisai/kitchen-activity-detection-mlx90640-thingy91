/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FS_MODULE_EVENT_H_
#define _FS_MODULE_EVENT_H_

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum fs_module_event_type {
  FS_EVT_ERROR
};

struct fs_module_event {
  struct event_header header;
  enum fs_module_event_type type;
  union {
    int err;
    int val;
  } data;
};

EVENT_TYPE_DECLARE(fs_module_event);

enum fs_data_module_event_type {
  FS_EVT_READ_FILE_REQ,
  FS_EVT_READ_FILE_RES,
  FS_EVT_WRITE_FILE,
  FS_EVT_APPEND_FILE,
  FS_EVT_DELETE_FILE,
};

struct fs_data_module_event {
  struct event_header header;
  enum fs_data_module_event_type type;
  char file_path[CONFIG_MAX_PATH_LENGTH];
  struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(fs_data_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _FS_MODULE_EVENT_H_ */
