/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CAMERA_MODULE_EVENT_H_
#define _CAMERA_MODULE_EVENT_H_

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum camera_module_event_type {
  CAMERA_EVT_TAKE_IMAGE,
  CAMERA_EVT_SET_ARDUCAM_JPEG_SIZE,
  CAMERA_EVT_SET_ARDUCAM_LIGHT_MODE,
  CAMERA_EVT_SET_ARDUCAM_COLOR_SATURATION,
  CAMERA_EVT_SET_ARDUCAM_BRIGHTNESS,
  CAMERA_EVT_SET_ARDUCAM_CONTRAST,
  CAMERA_EVT_SET_ARDUCAM_SPECIAL_EFFECTS,
  CAMERA_EVT_SET_ARDUCAM_TAKE_PICTURE,
  CAMERA_EVT_SET_ARDUCAM_JPEG_QUALITY,
  CAMERA_EVT_SET_ARDUCAM_CAM_INTERVAL,
  CAMERA_EVT_ERROR
};

struct camera_module_event {
  struct event_header header;
  enum camera_module_event_type type;
  union {
    bool bleTransfer;
    int err;
    int val;
  } data;
};

EVENT_TYPE_DECLARE(camera_module_event);

enum camera_module_data_event_type {
  CAMERA_DATA_EVT_DATA_READY,
};

struct camera_module_data_event {
  struct event_header header;
  enum camera_module_data_event_type type;
  size_t imageSize;
  bool isFirst;
  bool isLast;
  struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(camera_module_data_event);

#ifdef __cplusplus
}
#endif

#endif /* _CAMERA_MODULE_EVENT_H_ */
