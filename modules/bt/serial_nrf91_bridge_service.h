#ifndef BT_SERIAL_NRF91_BRIDGE_H_
#define BT_SERIAL_NRF91_BRIDGE_H_

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_UUID_SERIAL_NRF91_BRIDGE_VAL \
	BT_UUID_128_ENCODE(0xAAAF0A10, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERIAL_NRF91_BRIDGE_TX_VAL \
	BT_UUID_128_ENCODE(0xAAAF0A12, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERIAL_NRF91_BRIDGE_RX_VAL \
	BT_UUID_128_ENCODE(0xAAAF0A11, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERIAL_NRF91_BRIDGE_SERVICE   BT_UUID_DECLARE_128(BT_UUID_SERIAL_NRF91_BRIDGE_VAL)
#define BT_UUID_SERIAL_NRF91_BRIDGE_RX        BT_UUID_DECLARE_128(BT_UUID_SERIAL_NRF91_BRIDGE_RX_VAL)
#define BT_UUID_SERIAL_NRF91_BRIDGE_TX        BT_UUID_DECLARE_128(BT_UUID_SERIAL_NRF91_BRIDGE_TX_VAL)

enum bt_serial_nrf91_bridge_send_status {
	BT_SERIAL_NRF91_BRIDGE_SEND_STATUS_ENABLED,
	BT_SERIAL_NRF91_BRIDGE_SEND_STATUS_DISABLED,
};

struct bt_serial_nrf91_bridge_cb {

	void (*received)(struct bt_conn *conn,
			 const uint8_t *const data, uint16_t len);

	void (*sent)(struct bt_conn *conn);

	void (*send_enabled)(struct bt_conn *conn, enum bt_serial_nrf91_bridge_send_status status);

};

int bt_serial_nrf91_bridge_init(struct bt_serial_nrf91_bridge_cb *callbacks);
int bt_serial_nrf91_bridge_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

static inline uint32_t bt_serial_nrf91_bridge_get_mtu(struct bt_conn *conn)
{
	return bt_gatt_get_mtu(conn) - 3;
}

#ifdef __cplusplus
}
#endif

#endif /* BT_SERIAL_NRF91_BRIDGE_H_ */
