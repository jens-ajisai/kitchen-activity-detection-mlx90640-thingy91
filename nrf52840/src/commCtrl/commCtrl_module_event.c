/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "commCtrl_module_event.h"

#include <stdio.h>

static char* get_evt_type_str(enum commCtrl_module_event_type type) {
  switch (type) {
    case COMMCTRL_EVT_ERROR:
      return "COMMCTRL_EVT_ERROR";
    default:
      return "Unknown event";
  }
}

static int log_event(const struct event_header* eh, char* buf, size_t buf_len) {
  const struct commCtrl_module_event* event = cast_commCtrl_module_event(eh);
  return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(commCtrl_module_event, CONFIG_COMMCTRL_EVENTS_LOG, log_event, NULL);
