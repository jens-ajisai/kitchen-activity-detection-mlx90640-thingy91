/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "camera_module_event.h"

#include <stdio.h>

static char* get_evt_type_str(enum camera_module_event_type type) {
  switch (type) {
    case CAMERA_EVT_TAKE_IMAGE:
      return "CAMERA_EVT_TAKE_IMAGE";
    case CAMERA_EVT_ERROR:
      return "CAMERA_EVT_ERROR";
    case CAMERA_EVT_SET_ARDUCAM_JPEG_SIZE:
      return "CAMERA_EVT_SET_ARDUCAM_JPEG_SIZE";
    case CAMERA_EVT_SET_ARDUCAM_LIGHT_MODE:
      return "CAMERA_EVT_SET_ARDUCAM_LIGHT_MODE";
    case CAMERA_EVT_SET_ARDUCAM_COLOR_SATURATION:
      return "CAMERA_EVT_SET_ARDUCAM_COLOR_SATURATION";
    case CAMERA_EVT_SET_ARDUCAM_BRIGHTNESS:
      return "CAMERA_EVT_SET_ARDUCAM_BRIGHTNESS";
    case CAMERA_EVT_SET_ARDUCAM_CONTRAST:
      return "CAMERA_EVT_SET_ARDUCAM_CONTRAST";
    case CAMERA_EVT_SET_ARDUCAM_SPECIAL_EFFECTS:
      return "CAMERA_EVT_SET_ARDUCAM_SPECIAL_EFFECTS";
    case CAMERA_EVT_SET_ARDUCAM_TAKE_PICTURE:
      return "CAMERA_EVT_SET_ARDUCAM_TAKE_PICTURE";
    case CAMERA_EVT_SET_ARDUCAM_JPEG_QUALITY:
      return "CAMERA_EVT_SET_ARDUCAM_JPEG_QUALITY";
    case CAMERA_EVT_SET_ARDUCAM_CAM_INTERVAL:
      return "CAMERA_EVT_SET_ARDUCAM_CAM_INTERVAL";
    default:
      return "Unknown event";
  }
}

static int log_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct camera_module_event* event = cast_camera_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(camera_module_event, CONFIG_CAMERA_EVENTS_LOG, log_event, NULL);


static char* get_data_evt_type_str(enum camera_module_data_event_type type) {
  switch (type) {
    case CAMERA_DATA_EVT_DATA_READY:
      return "CAMERA_DATA_EVT_DATA_READY";
    default:
      return "Unknown event";
  }
}

static int log_data_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct camera_module_data_event* event = cast_camera_module_data_event(eh);
  return snprintf(buf, buf_len, "%s", get_data_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(camera_module_data_event, CONFIG_CAMERA_EVENTS_LOG, log_data_event, NULL);
