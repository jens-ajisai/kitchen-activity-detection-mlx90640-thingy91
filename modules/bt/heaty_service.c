#include "heaty_service.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <errno.h>
#include <logging/log.h>
#include <stddef.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(heaty_service, CONFIG_BLE_MODULE_LOG_LEVEL);

static bool notify_enabled;
static struct heaty_service_cb heaty_service_cb;

static void lc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  notify_enabled = (value == BT_GATT_CCC_NOTIFY);
  LOG_DBG("Notification enabled for heaty service");
}

static ssize_t write_set_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (heaty_service_cb.set_interval_cb) {
    uint8_t val = *((uint8_t *)buf);
    heaty_service_cb.set_interval_cb(val);
  }
  return len;
}

uint8_t interval_default = 1;
BT_GATT_SERVICE_DEFINE(heaty_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_SERVICE_HEATY),
                       BT_GATT_CHARACTERISTIC(BT_UUID_HEAT_MAP_TX,
                                              BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_CCC(lc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
                       BT_GATT_CHARACTERISTIC(BT_UUID_SET_INTERVAL,
                                              BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                                              BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL,
                                              write_set_interval, &interval_default), );

int heaty_service_init(struct heaty_service_cb *callbacks) {
  if (callbacks) {
    heaty_service_cb.set_interval_cb = callbacks->set_interval_cb;
  }

  return 0;
}

int heaty_service_send_heat_map(struct bt_conn *conn, const uint8_t* heat_map, uint16_t size) {
  const struct bt_gatt_attr *attr =
      bt_gatt_find_by_uuid(heaty_service.attrs, heaty_service.attr_count, BT_UUID_HEAT_MAP_TX);

  // do not use notify_enabled because it only works if only one connection is possible
  if (!conn) {
//    LOG_DBG("Notification send to all connected peers");
    return bt_gatt_notify_uuid(NULL, BT_UUID_HEAT_MAP_TX, heaty_service.attrs, heat_map, size);
  } else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
//    LOG_DBG("Notification send to conn");
    return bt_gatt_notify_uuid(conn, BT_UUID_HEAT_MAP_TX, heaty_service.attrs, heat_map, size);
  } else {
    // no error if listener is not subscribed
    return 0;

  }
}
