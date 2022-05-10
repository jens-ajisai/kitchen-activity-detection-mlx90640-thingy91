#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "serial_nrf91_bridge_service.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(bt_serial_nrf91_bridge, CONFIG_BLE_MODULE_LOG_LEVEL);

static struct bt_serial_nrf91_bridge_cb serial_cb;

static ssize_t serial_ccc_cfg_write(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	LOG_DBG("Notification has been turned %s",
		value == BT_GATT_CCC_NOTIFY ? "on" : "off");
	if (serial_cb.send_enabled) {
		serial_cb.send_enabled(conn, value == BT_GATT_CCC_NOTIFY ?
			BT_SERIAL_NRF91_BRIDGE_SEND_STATUS_ENABLED : BT_SERIAL_NRF91_BRIDGE_SEND_STATUS_DISABLED);
	}
  return sizeof(value);
}

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

  if (serial_cb.received) {
    serial_cb.received(conn, buf, len);
  }
  return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

  LOG_DBG("Data send, conn %p", (void *)conn);

  if (serial_cb.sent) {
    serial_cb.sent(conn);
  }
}

/* UART Service Declaration */
BT_GATT_SERVICE_DEFINE(serial_nrf91_bridge_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_SERIAL_NRF91_BRIDGE_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_SERIAL_NRF91_BRIDGE_TX,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       NULL, NULL, NULL),
    BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[]){
                            BT_GATT_CCC_INITIALIZER(NULL, serial_ccc_cfg_write, NULL)}),
                        (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),
	BT_GATT_CHARACTERISTIC(BT_UUID_SERIAL_NRF91_BRIDGE_RX,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, on_receive, NULL),
);

int bt_serial_nrf91_bridge_init(struct bt_serial_nrf91_bridge_cb *callbacks)
{
	if (callbacks) {
		serial_cb.received = callbacks->received;
		serial_cb.sent = callbacks->sent;
		serial_cb.send_enabled = callbacks->send_enabled;
	}

	return 0;
}

int bt_serial_nrf91_bridge_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	struct bt_gatt_notify_params params = {0};
	const struct bt_gatt_attr *attr = &serial_nrf91_bridge_svc.attrs[2];

	params.attr = attr;
	params.data = data;
	params.len = len;
	params.func = on_sent;

	if (!conn) {
		LOG_DBG("Notification send to all connected peers");
		return bt_gatt_notify_cb(NULL, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		LOG_DBG("Notification send to conn");
		return bt_gatt_notify_cb(conn, &params);
	} else {
		return -EINVAL;
	}
}
