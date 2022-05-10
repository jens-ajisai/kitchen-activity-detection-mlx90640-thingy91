#include "camera_service.h"

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

LOG_MODULE_REGISTER(camera_service, CONFIG_BLE_MODULE_LOG_LEVEL);

static bool notify_enabled;
static struct camera_service_cb camera_service_cb;

static void lc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t write_jpeg_size(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.jpeg_size_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.jpeg_size_cb(val);
  }
  return len;
}

static ssize_t write_light_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.light_mode_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.light_mode_cb(val);
  }
  return len;
}

static ssize_t write_color_saturation(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                      const void *buf, uint16_t len, uint16_t offset,
                                      uint8_t flags) {
  if (camera_service_cb.color_saturation_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.color_saturation_cb(val);
  }
  return len;
}

static ssize_t write_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.brightness_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.brightness_cb(val);
  }
  return len;
}

static ssize_t write_contrast(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.contrast_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.contrast_cb(val);
  }
  return len;
}

static ssize_t write_special_effects(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                     const void *buf, uint16_t len, uint16_t offset,
                                     uint8_t flags) {
  if (camera_service_cb.special_effects_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.special_effects_cb(val);
  }
  return len;
}

static ssize_t write_take_picture(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.take_picture_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.take_picture_cb(val);
  }
  return len;
}

static ssize_t write_jpeg_quality(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.jpeg_quality_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.jpeg_quality_cb(val);
  }
  return len;
}

static ssize_t write_cam_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
  if (camera_service_cb.cam_interval_cb) {
    uint8_t val = *((uint8_t *)buf);
    camera_service_cb.cam_interval_cb(val);
  }
  return len;
}

uint8_t take_picture_default = 0;
uint8_t jpeg_size_default = CONFIG_CAMERA_DEFAULT_JPEG_SIZE;
uint8_t light_mode_default = CONFIG_CAMERA_DEFAULT_LIGHT_MODE;
uint8_t color_saturation_default = CONFIG_CAMERA_DEFAULT_COLOR_SATURATION;
uint8_t brightness_default = CONFIG_CAMERA_DEFAULT_BRIGHTNESS;
uint8_t contrast_default = CONFIG_CAMERA_DEFAULT_CONTRAST;
uint8_t special_effects_default = CONFIG_CAMERA_DEFAULT_SPECIAL_EFFECTS;
uint8_t jpeg_quality_default = CONFIG_CAMERA_DEFAULT_JPEG_QUALITY;
uint8_t cam_interval_default = CONFIG_CAMERA_DEFAULT_CAM_INTERVAL;

BT_GATT_SERVICE_DEFINE(
    camera_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_SERVICE_CAMERA),
    BT_GATT_CHARACTERISTIC(BT_UUID_JPEG_SIZE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_jpeg_size,
                           &jpeg_size_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_LIGHT_MODE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_light_mode,
                           &light_mode_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_COLOR_SATURATION,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_color_saturation,
                           &color_saturation_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_BRIGHTNESS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_brightness,
                           &brightness_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_CONTRAST,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_contrast,
                           &contrast_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_SPECIAL_EFFECTS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_special_effects,
                           &special_effects_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_TAKE_PICTURE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_take_picture,
                           &take_picture_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_JPEG_QUALITY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_jpeg_quality,
                           &jpeg_quality_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_CAM_INTERVAL,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_cam_interval,
                           &cam_interval_default),
    BT_GATT_CHARACTERISTIC(BT_UUID_CAMERA_TX, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL, NULL),
    BT_GATT_CCC(lc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);

int camera_service_init(struct camera_service_cb *callbacks) {
  if (callbacks) {
    camera_service_cb.jpeg_size_cb = callbacks->jpeg_size_cb;
    camera_service_cb.light_mode_cb = callbacks->light_mode_cb;
    camera_service_cb.color_saturation_cb = callbacks->color_saturation_cb;
    camera_service_cb.brightness_cb = callbacks->brightness_cb;
    camera_service_cb.contrast_cb = callbacks->contrast_cb;
    camera_service_cb.special_effects_cb = callbacks->special_effects_cb;
    camera_service_cb.take_picture_cb = callbacks->take_picture_cb;
    camera_service_cb.jpeg_quality_cb = callbacks->jpeg_quality_cb;
    camera_service_cb.cam_interval_cb = callbacks->cam_interval_cb;
  }

  return 0;
}

int camera_service_send_picture(struct bt_conn *conn, const uint8_t *heat_map, uint16_t size) {
  const struct bt_gatt_attr *attr =
      bt_gatt_find_by_uuid(camera_service.attrs, camera_service.attr_count, BT_UUID_CAMERA_TX);

  // do not use notify_enabled because it only works if only one connection is possible
  if (!conn) {
    //    LOG_DBG("Notification send to all connected peers");
    return bt_gatt_notify_uuid(NULL, BT_UUID_CAMERA_TX, camera_service.attrs, heat_map, size);
  } else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
    //    LOG_DBG("Notification send to conn");
    return bt_gatt_notify_uuid(conn, BT_UUID_CAMERA_TX, camera_service.attrs, heat_map, size);
  } else {
    // no error if listener is not subscribed
    return 0;
  }
}
