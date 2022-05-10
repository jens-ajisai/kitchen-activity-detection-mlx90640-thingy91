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

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum sensors_module_event_type {
  SENSORS_EVT_ENV_TEMPERATURE,
  SENSORS_EVT_ENV_HUMIDITY,
  SENSORS_EVT_ENV_GAS,
  SENSORS_EVT_ERROR
};

struct sensors_module_event {
  struct event_header header;
  enum sensors_module_event_type type;
  union {
    double val;
    int err;
  } data;
};

EVENT_TYPE_DECLARE(sensors_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _SENSORS_MODULE_EVENT_H_ */
