/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "mqtt_module_event.h"

static char * get_evt_type_str(enum mqtt_module_event_type type)
{
    switch (type)
    {
        case MQTT_EVT_CONNECT:
            return "MQTT_EVT_CONNECT";
        case MQTT_EVT_CONNECTED:
            return "MQTT_EVT_CONNECTED";
        case MQTT_EVT_DISCONNECTED:
            return "MQTT_EVT_DISCONNECTED";
        case MQTT_EVT_SEND:
            return "MQTT_EVT_SEND";
        case MQTT_EVT_SUBSCRIBE:
            return "MQTT_EVT_SUBSCRIBE";
        case MQTT_EVT_RECEIVED:
            return "MQTT_EVT_RECEIVED";
        case MQTT_EVT_ERROR:
            return "MQTT_EVT_ERROR";
        default:
            return "Unknown event";
    }
}


static int log_event(const struct event_header * eh, char * buf, size_t buf_len)
{
    const struct mqtt_module_event * event = cast_mqtt_module_event(eh);


  if (event->type == MQTT_EVT_ERROR) {
    return snprintf(buf, buf_len, "%s - Error code %d", get_evt_type_str(event->type),
                    event->data.err);
  }
  return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

EVENT_TYPE_DEFINE(mqtt_module_event, CONFIG_MQTT_EVENTS_LOG, log_event, NULL);
