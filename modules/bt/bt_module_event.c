/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_module_event.h"

#include <stdio.h>

static char* get_bt_evt_type_str(enum bt_module_event_type type) {
  switch (type) {
#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE    
    case BT_EVT_SET_ARDUCAM_JPEG_SIZE:
      return "BT_EVT_SET_ARDUCAM_JPEG_SIZE";
    case BT_EVT_SET_ARDUCAM_LIGHT_MODE:
      return "BT_EVT_SET_ARDUCAM_LIGHT_MODE";
    case BT_EVT_SET_ARDUCAM_COLOR_SATURATION:
      return "BT_EVT_SET_ARDUCAM_COLOR_SATURATION";
    case BT_EVT_SET_ARDUCAM_BRIGHTNESS:
      return "BT_EVT_SET_ARDUCAM_BRIGHTNESS";
    case BT_EVT_SET_ARDUCAM_CONTRAST:
      return "BT_EVT_SET_ARDUCAM_CONTRAST";
    case BT_EVT_SET_ARDUCAM_SPECIAL_EFFECTS:
      return "BT_EVT_SET_ARDUCAM_SPECIAL_EFFECTS";
    case BT_EVT_SET_ARDUCAM_TAKE_PICTURE:
      return "BT_EVT_SET_ARDUCAM_TAKE_PICTURE";
    case BT_EVT_SET_ARDUCAM_JPEG_QUALITY:
      return "BT_EVT_SET_ARDUCAM_JPEG_QUALITY";
    case BT_EVT_SET_ARDUCAM_CAM_INTERVAL:
      return "BT_EVT_SET_ARDUCAM_CAM_INTERVAL";
#endif
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
    case BT_EVT_SET_HEATMAP_INTERVAL:
      return "BT_EVT_SET_HEATMAP_INTERVAL";
#endif
    case BT_EVT_START_ADVERTISING:
      return "BT_EVT_START_ADVERTISING";
    case BT_EVT_ERROR:
      return "BT_EVT_ERROR";
    default:
      return "Unknown event";
  }
}

static int log_bt_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct bt_module_event* event = cast_bt_module_event(eh);

  if (event->type == BT_EVT_ERROR) {
    return snprintf(buf, buf_len, "%s - Error code %d", get_bt_evt_type_str(event->type),
                    event->data.err);
  }
  return snprintf(buf, buf_len, "%s", get_bt_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(bt_module_event, CONFIG_BLE_EVENTS_LOG, log_bt_event, NULL);

static char* get_bt_data_evt_type_str(enum bt_data_module_event_type type) {
  switch (type) {
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE    
    case BT_EVT_HEATMAP_SEND:
      return "BT_EVT_HEATMAP_SEND";
#endif
#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
    case BT_EVT_CAMERA_SEND:
      return "BT_EVT_CAMERA_SEND";
#endif
    default:
      return "Unknown event";
  }
}

static int log_bt_data_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct bt_data_module_event* event = cast_bt_data_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_bt_data_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(bt_data_module_event, CONFIG_BLE_EVENTS_LOG, log_bt_data_event, NULL);