/*
 * Based on Application Event Manager which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MQTT_MODULE_EVENT_H_
#define _MQTT_MODULE_EVENT_H_

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mqtt_module_event_type {
	MQTT_EVT_CONNECT,
	MQTT_EVT_CONNECTED,
	MQTT_EVT_DISCONNECTED,
	MQTT_EVT_SEND,
	MQTT_EVT_SUBSCRIBE,
	MQTT_EVT_RECEIVED,
	MQTT_EVT_ERROR
};

struct mqtt_module_message {
	char* topic;
	char* message;
};

struct mqtt_module_event {
	struct event_header header;
	enum mqtt_module_event_type type;
	union {
		struct mqtt_module_message msg;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(mqtt_module_event);

#ifdef __cplusplus
}
#endif

#endif /* _MQTT_MODULE_EVENT_H_ */
