/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MODULE_EVENT_H_
#define _BT_MODULE_EVENT_H_

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum bt_module_event_type {
#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE  
  BT_EVT_SET_ARDUCAM_JPEG_SIZE,
  BT_EVT_SET_ARDUCAM_LIGHT_MODE,
  BT_EVT_SET_ARDUCAM_COLOR_SATURATION,
  BT_EVT_SET_ARDUCAM_BRIGHTNESS,
  BT_EVT_SET_ARDUCAM_CONTRAST,
  BT_EVT_SET_ARDUCAM_SPECIAL_EFFECTS,
  BT_EVT_SET_ARDUCAM_TAKE_PICTURE,
  BT_EVT_SET_ARDUCAM_JPEG_QUALITY,
  BT_EVT_SET_ARDUCAM_CAM_INTERVAL,
#endif
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
  BT_EVT_SET_HEATMAP_INTERVAL,
#endif
  BT_EVT_START_ADVERTISING,
  BT_EVT_ERROR
};

struct bt_module_event {
  struct event_header header;
  enum bt_module_event_type type;
  union {
    int err;
    int val;
  } data;
};

EVENT_TYPE_DECLARE(bt_module_event);

enum bt_data_module_event_type {
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
  BT_EVT_HEATMAP_SEND,
#endif
#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
  BT_EVT_CAMERA_SEND
#endif
};

struct bt_data_module_event {
  struct event_header header;
  enum bt_data_module_event_type type;
  bool useBase64;
  struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(bt_data_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _BT_MODULE_EVENT_H_ */
