/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensors_module_event.h"

#include <stdio.h>

static char* get_sensors_module_evt_type_str(enum sensors_module_event_type type) {
  switch (type) {
    case SENSORS_EVT_SET_INTERVAL:
      return "SENSORS_EVT_SET_INTERVAL";
    case SENSORS_EVT_ERROR:
      return "SENSORS_EVT_ERROR";
    default:
      return "Unknown event";
  }
}

static int log_sensors_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct sensors_module_event* event = cast_sensors_module_event(eh);

  if (event->type == SENSORS_EVT_ERROR) {
    return snprintf(buf, buf_len, "%s - Error code %d", get_sensors_module_evt_type_str(event->type),
                    event->data.err);
  }
  return snprintf(buf, buf_len, "%s", get_sensors_module_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(sensors_module_event, CONFIG_SENSORS_EVENTS_LOG, log_sensors_event, NULL);


static char* get_sensors_data_module_evt_type_str(enum sensors_data_module_event_type type) {
  switch (type) {
    case SENSORS_EVT_HEAT_MAP_DATA_READY:
      return "SENSORS_EVT_HEAT_MAP_DATA_READY";
    default:
      return "Unknown event";
  }
}

static int log_sensors_data_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct sensors_data_module_event* event = cast_sensors_data_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_sensors_data_module_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(sensors_data_module_event, CONFIG_SENSORS_EVENTS_LOG, log_sensors_data_event, NULL);
