/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSORS_MODULE_EVENT_H_
#define _SENSORS_MODULE_EVENT_H_

/**
 * @brief Sensor module event
 * @defgroup sensors_module_event Sensor module event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum sensors_module_event_type { SENSORS_EVT_SET_INTERVAL, SENSORS_EVT_ERROR };

struct sensors_module_event {
  struct event_header header;
  enum sensors_module_event_type type;
  union {
    int err;
    int val;
  } data;
};

EVENT_TYPE_DECLARE(sensors_module_event);

enum sensors_data_module_event_type {
  SENSORS_EVT_HEAT_MAP_DATA_READY,
};

struct sensors_data_module_event {
  struct event_header header;
  enum sensors_data_module_event_type type;
  int64_t timestamp;
  uint8_t sensorsId;
  struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(sensors_data_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSORS_MODULE_EVENT_H_ */
