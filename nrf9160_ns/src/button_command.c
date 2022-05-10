/*
 * The module architecture (using states, message queue in a separate thread, using events for communication)
 * is based on the Application Event Manager as used in the Asset Tracker v2 Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined CONFIG_CAF_BUTTONS

#define MODULE button_command

#include <event_manager.h>

#include <caf/events/button_event.h>
#include <logging/log.h>

#include "utils.h"
#include "common/modules_common.h"
LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

enum button_id {
    BUTTON_ID_0,
    BUTTON_ID_COUNT
};

static bool handle_button_event(const struct button_event *evt) {
  LOG_DBG("button_event pressed:%d, key_id:%d", evt->pressed, evt->key_id);
  if (evt->pressed) {
    switch (evt->key_id) {
      case BUTTON_ID_0:
        send_led_event(LED_TEST_BLUE);
        break;
      default:
        break;
    }
  } else {
    switch (evt->key_id) {
      case BUTTON_ID_0:
        send_led_event(LED_OFF);
        break;
      default:
        break;
    }
  }

  return false;
}

static bool event_handler(const struct event_header *eh) {
  LOG_DBG("event_handler");
  if (is_button_event(eh)) {
    LOG_DBG("event_handler is_button_event");
    struct button_event *event = cast_button_event(eh);
    handle_button_event(event);
  }
  return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, button_event);
#endif
