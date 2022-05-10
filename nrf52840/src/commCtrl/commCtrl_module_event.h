/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _COMMCTRL_MODULE_EVENT_H_
#define _COMMCTRL_MODULE_EVENT_H_

/**
 * @brief Sensor module event
 * @defgroup commCtrl_module_event Sensor module event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sensor event types submitted by Sensor module. */
enum commCtrl_module_event_type { COMMCTRL_EVT_ERROR };

/** @brief Sensor event. */
struct commCtrl_module_event {
  struct event_header header;
  enum commCtrl_module_event_type type;
  union {
    int err;
  } data;
};

EVENT_TYPE_DECLARE(commCtrl_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _COMMCTRL_MODULE_EVENT_H_ */
