/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_DATA_EVENT_H_
#define _BLE_DATA_EVENT_H_

#include <event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Peer connection event. */
struct ble_data_event {
	struct event_header header;

	uint8_t *buf;
	size_t len;
};

EVENT_TYPE_DECLARE(ble_data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_DATA_EVENT_H_ */
