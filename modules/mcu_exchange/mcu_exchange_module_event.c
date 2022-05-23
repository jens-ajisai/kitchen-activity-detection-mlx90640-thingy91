/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mcu_exchange_module_event.h"

#include <stdio.h>

static char* get_evt_type_str(enum mcu_exchange_module_event_type type) {
  switch (type) {
    case MCU_EXCHANGE_EVT_READY:
      return "MCU_EXCHANGE_EVT_READY";
    case MCU_EXCHANGE_EVT_DATA_READY:
      return "MCU_EXCHANGE_EVT_DATA_READY";
    case MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY:
      return "MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY";
    case MCU_EXCHANGE_EVT_BUTTON:
      return "MCU_EXCHANGE_EVT_BUTTON";
    case MCU_EXCHANGE_EVT_ERROR:
      return "MCU_EXCHANGE_EVT_ERROR";
    default:
      return "Unknown event";
  }
}

static int log_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct mcu_exchange_module_event* event = cast_mcu_exchange_module_event(eh);

  if (event->type == MCU_EXCHANGE_EVT_ERROR) {
    return snprintf(buf, buf_len, "%s - Error code %d", get_evt_type_str(event->type),
                    event->data.err);
  }
  return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(mcu_exchange_module_event, CONFIG_MCU_EXCHANGE_EVENTS_LOG, log_event, NULL);
