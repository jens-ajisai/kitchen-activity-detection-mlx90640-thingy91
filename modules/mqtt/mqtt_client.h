/*
 * This code is based on the nRF9160: Simple MQTT sample which has the below license
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/mqtt_simple/README.html
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include <zephyr.h>
#include <net/mqtt.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

typedef void (*mqtt_connected_cb_t)(const int result);
typedef void (*mqtt_disconnected_cb_t)(const int result);
typedef void (*mqtt_received_cb_t)(const int result, const uint8_t* data, const size_t len);


/** @brief Callback struct used by the module Service. */
struct mqtt_client_module_cb {
	mqtt_connected_cb_t			mqtt_connected_cb;
	mqtt_disconnected_cb_t			mqtt_disconnected_cb;
	mqtt_received_cb_t			mqtt_received_cb;
};

struct mqtt_client_info {
	const char* brokerHostname;
	int brokerPort;
	const char* clientId;
	const char* userName;
	const char* password;
	bool libtls;
    sec_tag_t* tlsSecTags;
	socklen_t tlsSecTagsLen;
    int secTag;
	int peerVerify;
    bool sessionCaching;
};

int mqtt_client_module_init(struct mqtt_client* client, struct mqtt_client_info *client_info, struct mqtt_client_module_cb *callbacks);
int mqtt_client_module_connect(struct mqtt_client* client);
int mqtt_client_module_disconnect(struct mqtt_client* client);
int mqtt_client_module_subscribe(struct mqtt_client* client, const char* topic);
int mqtt_client_module_data_publish(struct mqtt_client* client, const char* topic, uint8_t* data,
                                    size_t len);


#endif
