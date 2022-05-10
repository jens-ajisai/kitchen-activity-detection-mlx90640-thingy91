#ifndef __HEATY_SERVICE_H
#define __HEATY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#define BT_UUID_HEAT_SERVICE_VAL    BT_UUID_128_ENCODE(0xAAAF0900, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_HEAT_MAP_TX_VAL     BT_UUID_128_ENCODE(0xAAAF0901, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_SET_INTERVAL_VAL    BT_UUID_128_ENCODE(0xAAAF0902, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERVICE_HEATY       BT_UUID_DECLARE_128(BT_UUID_HEAT_SERVICE_VAL)
#define BT_UUID_HEAT_MAP_TX         BT_UUID_DECLARE_128(BT_UUID_HEAT_MAP_TX_VAL)
#define BT_UUID_SET_INTERVAL        BT_UUID_DECLARE_128(BT_UUID_SET_INTERVAL_VAL)

typedef void (*set_interval_cb_t)(const uint8_t value);

struct heaty_service_cb {
	set_interval_cb_t			set_interval_cb;
};

int heaty_service_init(struct heaty_service_cb *callbacks);

int heaty_service_send_heat_map(struct bt_conn *conn, const uint8_t* heat_map, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* __HEATY_SERVICE_H */
