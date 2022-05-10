#define MODULE control_module

#include <event_manager.h>
#include <logging/log.h>
#include <stdio.h>
#include <zephyr.h>

#include "bt/bt_module_event.h"
#include "camera/camera_module_event.h"
#include "common/modules_common.h"
#ifdef CONFIG_FS_MODULE
#include "fs/fs_module_event.h"
#endif
#include "sensors/sensors_module_event.h"
#include "util_settings.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
static void sendEvToBle(const char* buf, uint32_t size, bool useBase64) {
  struct bt_data_module_event* bt_event = new_bt_data_module_event(size);
  bt_event->type = BT_EVT_CAMERA_SEND;
  bt_event->dyndata.size = size;
  memcpy(bt_event->dyndata.data, buf, size);
  bt_event->useBase64 = useBase64;
  EVENT_SUBMIT(bt_event);
}

#define MAX_HEADER_LENGTH (CONFIG_MAX_PATH_LENGTH + 19 + 6)

static void sendHeader(const char* filename, uint32_t imageSize) {
  LOG_INF("send bluetooth header");
  static char header[MAX_HEADER_LENGTH];

  memset(header, 0, MAX_HEADER_LENGTH);
  snprintf(header, MAX_HEADER_LENGTH, "{\"name\":\"%s\",\"size\":%d}", filename, imageSize);
  sendEvToBle(header, strlen(header), false);
}

static void sendFinal() {
  char buf[1] = {0};
  sendEvToBle(buf, 1, false);
}

static bool handle_bt_module_event(const struct bt_module_event* event) {
  if (event->type == BT_EVT_SET_ARDUCAM_JPEG_SIZE) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_JPEG_SIZE, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_LIGHT_MODE) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_LIGHT_MODE, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_COLOR_SATURATION) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_COLOR_SATURATION, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_BRIGHTNESS) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_BRIGHTNESS, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_CONTRAST) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_CONTRAST, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_SPECIAL_EFFECTS) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_SPECIAL_EFFECTS, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_TAKE_PICTURE) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_TAKE_PICTURE, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_JPEG_QUALITY) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_JPEG_QUALITY, event->data.val);
  }
  if (event->type == BT_EVT_SET_ARDUCAM_CAM_INTERVAL) {
    SEND_VAL(camera, CAMERA_EVT_SET_ARDUCAM_CAM_INTERVAL, event->data.val);
  }
  return false;
}
#endif

static const char* counter_key = "counter";
static char filename[CONFIG_MAX_PATH_LENGTH];

#ifdef CONFIG_DATE_TIME
#include <date_time.h>
#endif

static bool handle_camera_module_data_event(const struct camera_module_data_event* event) {
  LOG_DBG("handle_camera_module_data_event");

  if (event->type == CAMERA_DATA_EVT_DATA_READY) {
#ifdef CONFIG_DATE_TIME
    struct tm* tm_localtime;
    int64_t t = 0;
    int ret = date_time_now(&t);
    if (ret < 0) {
      t = k_uptime_get();
    }
    t /= 1000;
    tm_localtime = localtime(&t);
    sprintf(filename, "%02u-%02u-%02u-%02u-%02u.jpg", tm_localtime->tm_mon + 1,
            tm_localtime->tm_mday, tm_localtime->tm_hour, tm_localtime->tm_min,
            tm_localtime->tm_sec);
#endif

    if (event->isFirst) {
#ifndef CONFIG_DATE_TIME
      incrementCounter(counter_key);
      sprintf(filename, "%05d.jpg", getCounter(counter_key));
#endif

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
      sendHeader(filename, event->imageSize);
#endif
    } else {
#ifndef CONFIG_DATE_TIME
      sprintf(filename, "%05d.jpg", getCounter(counter_key));
#endif
    }

    struct fs_data_module_event* fs_event = new_fs_data_module_event(event->dyndata.size);
    memcpy(fs_event->dyndata.data, event->dyndata.data, event->dyndata.size);
    fs_event->dyndata.size = event->dyndata.size;
    fs_event->type = (event->isFirst) ? FS_EVT_WRITE_FILE : FS_EVT_APPEND_FILE;
    memcpy(fs_event->file_path, filename, CONFIG_MAX_PATH_LENGTH);

#ifdef CONFIG_BLE_ENABLE_CAMERA_SERVICE
    sendEvToBle(event->dyndata.data, event->dyndata.size, true);
    if (event->isLast) {
      sendFinal();
    }
#endif

    EVENT_SUBMIT(fs_event);
  }
  return false;
}

static bool handle_sensors_data_module_event(const struct sensors_data_module_event* event) {
  if (event->type == SENSORS_EVT_HEAT_MAP_DATA_READY) {
#ifdef CONFIG_BLE_ENABLE_HEATY_SERVICE
    {
      struct bt_data_module_event* bt_event = new_bt_data_module_event(event->dyndata.size);
      bt_event->type = BT_EVT_HEATMAP_SEND;
      bt_event->dyndata.size = event->dyndata.size;
      memcpy(bt_event->dyndata.data, event->dyndata.data, event->dyndata.size);
      bt_event->useBase64 = true;
      EVENT_SUBMIT(bt_event);
    }
    {
      struct bt_data_module_event* bt_event = new_bt_data_module_event(1);
      bt_event->type = BT_EVT_HEATMAP_SEND;
      bt_event->dyndata.size = 1;
      memset(bt_event->dyndata.data, 0, 1);
      bt_event->useBase64 = false;
      EVENT_SUBMIT(bt_event);
    }
#endif

#ifdef CONFIG_FS_MODULE
    {
#ifdef CONFIG_DATE_TIME
      struct tm* tm_localtime;
      int64_t t = 0;
      int ret = date_time_now(&t);
      if (ret < 0) {
        t = k_uptime_get();
      }
      t /= 1000;
      tm_localtime = localtime(&t);
      sprintf(filename, "%02u-%02u-%02u-%02u-%02u.map", tm_localtime->tm_mon + 1,
              tm_localtime->tm_mday, tm_localtime->tm_hour, tm_localtime->tm_min,
              tm_localtime->tm_sec);
#else
      sprintf(filename, "%04d.map", getCounter(counter_key));
#endif
      struct fs_data_module_event* fs_event = new_fs_data_module_event(event->dyndata.size);
      fs_event->type = FS_EVT_WRITE_FILE;
      memcpy(fs_event->file_path, filename, CONFIG_MAX_PATH_LENGTH);
      memcpy(fs_event->dyndata.data, event->dyndata.data, event->dyndata.size);
      fs_event->dyndata.size = event->dyndata.size;
      EVENT_SUBMIT(fs_event);
    }
#endif
  }
  return false;
}

static bool event_handler(const struct event_header* eh) {
  if (is_camera_module_data_event(eh)) {
    return handle_camera_module_data_event(cast_camera_module_data_event(eh));
  }

  if (is_bt_module_event(eh)) {
    return handle_bt_module_event(cast_bt_module_event(eh));
  }

  if (is_sensors_data_module_event(eh)) {
    return handle_sensors_data_module_event(cast_sensors_data_module_event(eh));
  }

  LOG_WRN("unhandled event.");
  return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, camera_module_data_event);
EVENT_SUBSCRIBE(MODULE, bt_module_event);
EVENT_SUBSCRIBE(MODULE, sensors_data_module_event);
