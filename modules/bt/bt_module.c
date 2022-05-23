/*
 * The module architecture (using states, message queue in a separate thread, using events for
 * communication) is based on the Application Event Manager as used in the Asset Tracker v2
 * Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE bt_module

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

/* MCUMgr BT FOTA includes */
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
#include <mgmt/mcumgr/smp_bt.h>
#endif

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <bluetooth/uuid.h>
#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <shell/shell_bt_nus.h>
#include <sys/base64.h>
#include <zephyr.h>

#include "bt/bt_module_event.h"
#include "common/memory_hook.h"
#include "common/modules_common.h"
#include "heaty_service.h"
#include "time_service.h"

#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
#include "commCtrl/bridge/events/ble_data_event.h"
#include "commCtrl/bridge/events/peer_conn_event.h"
#include "commCtrl/bridge/events/uart_data_event.h"
#include "serial_nrf91_bridge_service.h"
#endif

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
#include "camera_service.h"
#endif

LOG_MODULE_REGISTER(MODULE, CONFIG_BLE_MODULE_LOG_LEVEL);

struct bt_msg_data {
  union {
    struct module_state_event module_state;
    struct bt_module_event bt;
    struct bt_data_module_event bt_data;
#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
    struct ble_data_event ble_data;
    struct uart_data_event uart_data;
#endif
  } module;
  struct event_dyndata *dyndata;
};

/* Cam Control module message queue. */
#define BT_QUEUE_ENTRY_COUNT 15
#define BT_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_bt, sizeof(struct bt_msg_data), BT_QUEUE_ENTRY_COUNT, BT_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "bt",
    .msg_q = &msgq_bt,
};
static enum state_type {
  STATE_INIT,
  STATE_ADVERTISING,
  STATE_DISCONNECTED,
  STATE_CONNECTED,
  STATE_CONNECTED_ADVERTISING,
} state;

static char *state2str(enum state_type new_state) {
  switch (new_state) {
    case STATE_INIT:
      return "STATE_INIT";
    case STATE_ADVERTISING:
      return "STATE_ADVERTISING";
    case STATE_DISCONNECTED:
      return "STATE_DISCONNECTED";
    case STATE_CONNECTED:
      return "STATE_CONNECTED";
    case STATE_CONNECTED_ADVERTISING:
      return "STATE_CONNECTED_ADVERTISING";
    default:
      return "Unknown";
  }
}

DECLARE_SET_STATE()

static bool event_handler(const struct event_header *eh) {
  struct bt_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(bt_module, bt)
  TRANSLATE_EVENT_TO_MSG(bt_data_module, bt_data)

#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
  TRANSLATE_EVENT_TO_MSG(uart_data, uart_data)

  if (is_uart_data_event(eh)) {
    struct uart_data_event *event = cast_uart_data_event(eh);
    msg.dyndata =
        my_malloc(sizeof(struct event_dyndata) + event->len, HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata->data, event->buf, event->len);
    msg.dyndata->size = event->len;
  }

  if (is_ble_data_event(eh)) {
    const struct ble_data_event *event = cast_ble_data_event(eh);
    my_free(event->buf);
  }
#endif

  if (is_module_state_event(eh)) {
    struct module_state_event *event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  if (is_bt_data_module_event(eh)) {
    struct bt_data_module_event *event = cast_bt_data_module_event(eh);
    msg.dyndata = my_malloc(sizeof(struct event_dyndata) + event->dyndata.size,
                            HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata, &event->dyndata, sizeof(struct event_dyndata) + event->dyndata.size);
  }

  ENQUEUE_MSG(bt, BT)

  return false;
}

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

struct bt_conn_myInfo {
  struct bt_conn *conn;
  bool serial_notifications;
#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
  bool serial_nrf91_bridge_notifications;
#endif
  uint16_t mtu;
};

static struct bt_conn_myInfo conns[CONFIG_BT_MAX_CONN] = {
#if CONFIG_BT_MAX_CONN >= 1
    {.conn = NULL,
     .serial_notifications = false,
#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
     .serial_nrf91_bridge_notifications = false,
#endif
     .mtu = 0}
#endif
#if CONFIG_BT_MAX_CONN >= 2
    ,
    {.conn = NULL,
     .serial_notifications = false,
#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
     .serial_nrf91_bridge_notifications = false,
#endif
     .mtu = 0}
#endif
};
static uint8_t conn_cnt = 0;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL)};

static struct k_work advertise_work;

static void advertise(struct k_work *work) {
  if (state == STATE_INIT || state == STATE_DISCONNECTED || state == STATE_CONNECTED) {
    int err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_1,
                                              BT_GAP_ADV_FAST_INT_MAX_2,
                                              // BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX,
                                              NULL),
                              ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
      LOG_ERR("Advertising failed to start (err %d)", err);
      SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_ADV_FAILED);
    }

    // need to change this when advertising for the second device
    if (conn_cnt > 0) {
      state_set(STATE_CONNECTED_ADVERTISING);
    } else {
      state_set(STATE_ADVERTISING);
    }

    LOG_DBG("Advertising successfully started");
  }
}

static struct bt_gatt_exchange_params exchange_params;

static void exchange_func(struct bt_conn *conn, uint8_t err,
                          struct bt_gatt_exchange_params *params) {
  if (!err) {
    for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
      if (conns[i].conn == conn) {
        conns[i].mtu = bt_gatt_get_mtu(conn) - 3;
        LOG_INF("conns[%d]->mtu, %d", i, conns[i].mtu);

        // this should be a number dividable by 4 + 1 for string terminator
        while (conns[i].mtu % 4 != 1) (conns[i].mtu)--;
        LOG_INF("conns[%d]->mtu fixed %d", i, conns[i].mtu);
        break;
      }
    }
  }
}

static void connected(struct bt_conn *conn, uint8_t err) {
  state_set(STATE_CONNECTED);

  char addr[BT_ADDR_LE_STR_LEN];

  if (err) {
    LOG_WRN("Connection failed (err %u)", err);
    return;
  }

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Connected %s", log_strdup(addr));

  exchange_params.func = exchange_func;

  err = bt_gatt_exchange_mtu(conn, &exchange_params);
  if (err) {
    LOG_WRN("bt_gatt_exchange_mtu: %d", err);
  }

  for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
    if (!conns[i].conn) {
      conns[i].conn = bt_conn_ref(conn);
      conns[i].mtu = 0;
      conns[i].serial_notifications = false;
      break;
    }
  }

  // limitation that bt_nus shell supports only one connection.
  // therefore connect the shell last and disconnect first.
#ifdef CONFIG_BT_NUS
  shell_bt_nus_enable(conn);
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

  conn_cnt--;
  if (!conn_cnt) state_set(STATE_DISCONNECTED);

  for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
    if (conns[i].conn == conn) {
      bt_conn_unref(conns[i].conn);
      conns[i].conn = NULL;
      conns[i].mtu = 0;
      conns[i].serial_notifications = false;
      break;
    }
  }

  // limitation that bt_nus shell supports only one connection.
  // therefore connect the shell last and disconnect first.
#ifdef CONFIG_BT_NUS
  shell_bt_nus_disable();
#endif

  k_work_submit(&advertise_work);
}

#ifdef CONFIG_BLE_LBS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (!err) {
    LOG_DBG("Security changed: %s level %u", addr, level);
  } else {
    LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_SECURITY_FAILED);
  }
}
#endif

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
#ifdef CONFIG_BLE_LBS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

#if defined(CONFIG_BLE_LBS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Pairing cancelled: %s", addr);
}

static void pairing_confirm(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  bt_conn_auth_pairing_confirm(conn);

  LOG_DBG("Pairing confirmed: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_DBG("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {.passkey_display = auth_passkey_display,
                                                     .cancel = auth_cancel,
                                                     .pairing_confirm = pairing_confirm,
                                                     .pairing_complete = pairing_complete,
                                                     .pairing_failed = pairing_failed};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
#endif

#if defined(CONFIG_MCUMGR_SMP_BT)
static int software_update_confirmation_handler(uint32_t offset, uint32_t size, void *arg) {
  /* For now just print update progress and confirm data chunk without any
   * additional checks.
   */
  LOG_INF("Device firmware upgrade progress %d B / %d B", offset, size);

  return 0;
}
#endif

#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
static void set_heatmap_interval_cb(uint8_t val) {
  LOG_DBG("set_interval_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_HEATMAP_INTERVAL, val);
}

static struct heaty_service_cb heaty_service_callbacs = {
    .set_interval_cb = set_heatmap_interval_cb,
};
#endif

#ifdef CONFIG_BLE_ENABLE_TIME_SERVICE
#include <date_time.h>

static void set_time_cb(time_t val) {
  struct tm *tm_localtime;
  tm_localtime = localtime(&val);

  date_time_set(tm_localtime);
  LOG_DBG("Set time to %s %04u (%lli)", tm_localtime->tm_year + 1900, asctime(tm_localtime), val);
}

static struct time_service_cb time_service_callbacs = {
    .set_time_cb = set_time_cb,
};
#endif

#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
static void bt_receive_nrf9160_bridge_cb(struct bt_conn *conn, const uint8_t *const data,
                                         uint16_t len) {
  /*
    char addr[BT_ADDR_LE_STR_LEN] = {0};

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

    LOG_DBG("Received data from: %s", addr);
    LOG_HEXDUMP_DBG(data, len, "Data");
  */

  void *buf = my_malloc(len, HEAP_MEMORY_STATISTICS_ID_BLE_DATA);
  memcpy(buf, data, len);

  struct ble_data_event *event = new_ble_data_event();
  event->buf = buf;
  event->len = len;
  EVENT_SUBMIT(event);
}

static void bt_send_nrf9160_bridge_enabled_cb(struct bt_conn *conn,
                                              enum bt_serial_nrf91_bridge_send_status status) {
  char addr[BT_ADDR_LE_STR_LEN] = {0};
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
    if (conns[i].conn == conn) {
      if (status == BT_SERIAL_NRF91_BRIDGE_SEND_STATUS_ENABLED) {
        LOG_INF("%s listening to serial_nrf9160_bridge_service", addr);
        conns[i].serial_nrf91_bridge_notifications = true;

        struct peer_conn_event *event = new_peer_conn_event();
        event->peer_id = PEER_ID_BLE;
        event->dev_idx = 0;
        event->baudrate = 0; /* Don't care */
        event->conn_state = PEER_STATE_CONNECTED;
        EVENT_SUBMIT(event);
      } else {
        LOG_INF("remove listener %s from serial_nrf9160_bridge_service", addr);
        conns[i].serial_nrf91_bridge_notifications = false;

        struct peer_conn_event *event = new_peer_conn_event();
        event->peer_id = PEER_ID_BLE;
        event->dev_idx = 0;
        event->baudrate = 0; /* Don't care */
        event->conn_state = PEER_STATE_DISCONNECTED;
        EVENT_SUBMIT(event);
      }
      break;
    }
  }
}

static struct bt_serial_nrf91_bridge_cb serial_nrf91_bridge_cb = {
    .received = bt_receive_nrf9160_bridge_cb,
    .send_enabled = bt_send_nrf9160_bridge_enabled_cb,
};
#endif

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
static void jpeg_size_cb(uint8_t val) {
  LOG_DBG("jpeg_size_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_JPEG_SIZE, val);
}

static void light_mode_cb(uint8_t val) {
  LOG_DBG("light_mode_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_LIGHT_MODE, val);
}

static void color_saturation_cb(uint8_t val) {
  LOG_DBG("color_saturation_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_COLOR_SATURATION, val);
}

static void brightness_cb(uint8_t val) {
  LOG_DBG("brightness_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_BRIGHTNESS, val);
}

static void contrast_cb(uint8_t val) {
  LOG_DBG("contrast_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_CONTRAST, val);
}

static void special_effects_cb(uint8_t val) {
  LOG_DBG("special_effects_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_SPECIAL_EFFECTS, val);
}

static void take_picture_cb(bool val) {
  LOG_DBG("take_picture_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_TAKE_PICTURE, val);
}

static void jpeg_quality_cb(uint8_t val) {
  LOG_DBG("jpeg_quality_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_JPEG_QUALITY, val);
}

static void cam_interval_cb(uint8_t val) {
  LOG_DBG("cam_interval_cb(%d)", val);
  SEND_VAL(bt, BT_EVT_SET_ARDUCAM_CAM_INTERVAL, val);
}

static struct camera_service_cb camera_service_callbacs = {
    .jpeg_size_cb = jpeg_size_cb,
    .light_mode_cb = light_mode_cb,
    .color_saturation_cb = color_saturation_cb,
    .brightness_cb = brightness_cb,
    .contrast_cb = contrast_cb,
    .special_effects_cb = special_effects_cb,
    .take_picture_cb = take_picture_cb,
    .jpeg_quality_cb = jpeg_quality_cb,
    .cam_interval_cb = cam_interval_cb,
};
#endif

static void bt_ready(int err) {
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return;
  }

  if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
    settings_load();
  }

  LOG_DBG("Bluetooth initialized");

#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
  err = heaty_service_init(&heaty_service_callbacs);
  if (err) {
    LOG_ERR("Failed to init heaty service (err:%d)", err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_HEATY_INIT_FAILED);
    return;
  }
#endif

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
  err = camera_service_init(&camera_service_callbacs);
  if (err) {
    LOG_ERR("Failed to init camera service (err:%d)", err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_CAMERA_INIT_FAILED);
    return;
  }
#endif

  LOG_DBG("Heaty service initialized");

#ifdef CONFIG_BLE_ENABLE_TIME_SERVICE
  err = time_service_init(&time_service_callbacs);
  if (err) {
    LOG_ERR("Failed to initialize time service (err: %d)", err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_TIME_INIT_FAILED);
    return;
  }
#endif

#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
  err = bt_serial_nrf91_bridge_init(&serial_nrf91_bridge_cb);
  if (err) {
    LOG_ERR("Failed to initialize UART service (err: %d)", err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_BRIDGE_INIT_FAILED);
    return;
  }
#endif

  LOG_DBG("Bt serial service initialized");

#ifdef CONFIG_BT_NUS
  err = shell_bt_nus_init();
  if (err) {
    LOG_ERR("Failed to initialize BT NUS shell (err: %d)", err);
    SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_BT_NUS_INIT_FAILED);
    return;
  }
#endif

  LOG_DBG("Shell bt nus service initialized");

#if defined(CONFIG_MCUMGR_SMP_BT) && defined(CONFIG_MCUMGR_CMD_IMG_MGMT)
  img_mgmt_set_upload_cb(software_update_confirmation_handler, NULL);
  smp_bt_register();
#endif

#ifdef CONFIG_BLE_AUTOSTART_ADVERTISING
  k_work_submit(&advertise_work);
#endif
  state_set(STATE_DISCONNECTED);
}

static void init_module() {
  k_work_init(&advertise_work, advertise);

  bt_conn_cb_register(&conn_callbacks);
  if (IS_ENABLED(CONFIG_BLE_LBS_SECURITY_ENABLED)) {
    LOG_DBG("CONFIG_BLE_LBS_SECURITY_ENABLED");
    bt_conn_auth_cb_register(&conn_auth_callbacks);
  }

  bt_enable(bt_ready);
}

static void on_state_init(struct bt_msg_data *msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    // workaround: Sending errors before UART line of mcu exchange is ready drops the message
    // TODO: establish correct startup sequence, but take modularity and dependencies into account
    k_sleep(K_SECONDS(1));
    LOG_DBG("  *** Initialize BT ***");
    init_module();
  }
}

#define BASE64_ENCODE_LEN(bin_len) (4 * (((bin_len) + 2) / 3))
#define BASE64_DECODE_LEN(bin_len) (((bin_len)*3) / 4)

static void serialSend(struct bt_conn_myInfo *bt_conn_thisInfo, uint8_t *data, uint16_t bytesToSend,
                       bool useBase64,
                       int (*serialSend_cb)(struct bt_conn *conn, const uint8_t *data,
                                            uint16_t len)) {
  if (state == STATE_CONNECTED || state == STATE_CONNECTED_ADVERTISING) {
    char chunk[bt_conn_thisInfo->mtu];
    uint32_t decoded_chunk_size = BASE64_DECODE_LEN(bt_conn_thisInfo->mtu - 1);

    uint32_t transactionLength = 0;
    char *pos = data;

    for (uint16_t offset = 0; offset < bytesToSend; offset += transactionLength) {
      size_t newSize = 0;
      if (useBase64) {
#pragma GCC diagnostic ignored "-Wsign-compare"
        transactionLength = (bytesToSend - offset) > decoded_chunk_size ? decoded_chunk_size
                                                                        : (bytesToSend - offset);
#pragma GCC diagnostic pop

        int ret = base64_encode(chunk, bt_conn_thisInfo->mtu, &newSize, pos, transactionLength);
        // do not send the null terminator over ble as more chunks might follow.
        if (ret) {
          LOG_ERR("base64_encode error: %d", ret);
          SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_BASE64_FAILED);
        }
      } else {
        transactionLength = (bytesToSend - offset) > bt_conn_thisInfo->mtu ? bt_conn_thisInfo->mtu
                                                                           : (bytesToSend - offset);
        memcpy(chunk, pos, transactionLength);
        newSize = transactionLength;
      }

      LOG_DBG("bt_serial_send %d/%d", offset, bytesToSend);
      int ret = serialSend_cb(bt_conn_thisInfo->conn, chunk, newSize);
      k_sleep(K_MSEC(5));
      if (ret) {
        LOG_ERR("Failed to send data over BLE connection. Err=%d", ret);
        SEND_ERROR(bt, BT_EVT_ERROR, BT_EVT_ERR_SEND_FAILED);
      }
      pos += transactionLength;
    }
  }
}

static void on_state_connected(struct bt_msg_data *msg) {
  if ((IS_EVENT(msg, bt, BT_EVT_START_ADVERTISING)) ||
      (IS_EVENT(msg, bt, BT_EVT_TOGGLE_ADVERTISING))) {
    k_work_submit(&advertise_work);
  }

#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
  if (IS_EVENT(msg, bt_data, BT_EVT_HEATMAP_SEND)) {
    for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
      if (conns[i].conn) {
        LOG_INF("BT_EVT_HEATMAP_SEND Conn %d", i);
        serialSend(&conns[i], msg->dyndata->data, msg->dyndata->size, msg->module.bt_data.useBase64,
                   &heaty_service_send_heat_map);
      }
    }
  }
#endif

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
  if (IS_EVENT(msg, bt_data, BT_EVT_CAMERA_SEND)) {
    for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
      if (conns[i].conn) {
        LOG_INF("BT_EVT_CAMERA_SEND Conn %d", i);
        serialSend(&conns[i], msg->dyndata->data, msg->dyndata->size, msg->module.bt_data.useBase64,
                   &camera_service_send_picture);
      }
    }
  }
#endif

#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
  if (is_uart_data_event(&msg->module.uart_data.header)) {
    for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
      if (conns[i].serial_nrf91_bridge_notifications) {
        serialSend(&conns[i], msg->dyndata->data, msg->dyndata->size, false,
                   &bt_serial_nrf91_bridge_send);
      }
    }
  }
#endif
}

static void on_state_connected_advertising(struct bt_msg_data *msg) {
  if (IS_EVENT(msg, bt, BT_EVT_TOGGLE_ADVERTISING)) {
    bt_le_adv_stop();
    state_set(STATE_CONNECTED);
  } else {
    on_state_connected(msg);
  }
}

static void on_state_advertising(struct bt_msg_data *msg) {
  if (IS_EVENT(msg, bt, BT_EVT_TOGGLE_ADVERTISING)) {
    bt_le_adv_stop();
    state_set(STATE_DISCONNECTED);
  }
}

static void on_state_disconnected(struct bt_msg_data *msg) {
  if ((IS_EVENT(msg, bt, BT_EVT_START_ADVERTISING)) ||
      (IS_EVENT(msg, bt, BT_EVT_TOGGLE_ADVERTISING))) {
    k_work_submit(&advertise_work);
  }
}

static void on_all_states(struct bt_msg_data *msg) {
  if (msg->dyndata) {
    my_free(msg->dyndata);
  }
}

static void module_thread_fn(void) {
  struct bt_msg_data msg;
  self.thread_id = k_current_get();
  module_start(&self);
  state_set(STATE_INIT);

  while (true) {
    module_get_next_msg(&self, &msg);

    switch (state) {
      case STATE_INIT:
        on_state_init(&msg);
        break;
      case STATE_CONNECTED:
        on_state_connected(&msg);
        break;
      case STATE_CONNECTED_ADVERTISING:
        on_state_connected_advertising(&msg);
        break;
      case STATE_ADVERTISING:
        on_state_advertising(&msg);
        break;
      case STATE_DISCONNECTED:
        on_state_disconnected(&msg);
        break;
      default:

        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(ble_module_thread, CONFIG_BLE_THREAD_STACK_SIZE, module_thread_fn, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, bt_module_event);
EVENT_SUBSCRIBE(MODULE, bt_data_module_event);
#ifdef CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE
EVENT_SUBSCRIBE(MODULE, uart_data_event);
EVENT_SUBSCRIBE_FINAL(MODULE, ble_data_event);
#endif
EVENT_SUBSCRIBE(MODULE, module_state_event);
