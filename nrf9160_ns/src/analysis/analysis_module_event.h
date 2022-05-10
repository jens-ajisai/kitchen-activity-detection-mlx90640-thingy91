/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ANALYSIS_MODULE_EVENT_H_
#define _ANALYSIS_MODULE_EVENT_H_

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum analysis_module_event_type {
  ANALYSIS_EVT_ERROR
};

struct analysis_module_event {
  struct event_header header;
  enum analysis_module_event_type type;
  union {
    int val;
    int err;
  } data;
};

EVENT_TYPE_DECLARE(analysis_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _ANALYSIS_MODULE_EVENT_H_ */
