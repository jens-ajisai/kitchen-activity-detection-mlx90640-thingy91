/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MCU_EXCHANGE_MODULE_EVENT_H_
#define _MCU_EXCHANGE_MODULE_EVENT_H_

#include "event_manager.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mcu_exchange_module_event_type {
  MCU_EXCHANGE_EVT_READY,
  MCU_EXCHANGE_EVT_DATA_READY,
  MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY,
  MCU_EXCHANGE_EVT_BUTTON,
  MCU_EXCHANGE_EVT_ERROR
};

struct mcu_exchange_module_event {
  struct event_header header;
  enum mcu_exchange_module_event_type type;
  bool received;
  union {
    int err;
  } data;
  struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(mcu_exchange_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _MCU_EXCHANGE_EVENT_H_ */
