/*
 * The module architecture (using states, message queue in a separate thread, using events for communication)
 * is based on the Application Event Manager as used in the Asset Tracker v2 Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <event_manager.h>
#include <zephyr.h>

#define MODULE camera_module

#include <caf/events/module_state_event.h>
#include <logging/log.h>
#include <stdio.h>

#include "camera/camera_module_event.h"
#include "common/modules_common.h"
#include "arducam_mini_2mp/ArducamMini2MP.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_CAMERA_MODULE_LOG_LEVEL);

#define CAMERA_WORK_THREAD_STACK_SIZE CONFIG_CAMERA_WORK_THREAD_STACK_SIZE
#define CAMERA_WORK_THREAD_PRIORITY K_HIGHEST_APPLICATION_THREAD_PRIO

K_THREAD_STACK_DEFINE(camera_work_stack_area, CAMERA_WORK_THREAD_STACK_SIZE);
struct k_work_q camera_work_q;
static struct k_work_delayable read_camera_work;

uint16_t camera_interval = CONFIG_CAMERA_DEFAULT_CAM_INTERVAL;

struct camera_msg_data {
  union {
    struct module_state_event module_state;
    struct camera_module_event camera;
  } module;
};

/* Diag module message queue. */
#define CAMERA_QUEUE_ENTRY_COUNT 10
#define CAMERA_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_camera, sizeof(struct camera_msg_data), CAMERA_QUEUE_ENTRY_COUNT,
              CAMERA_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "camera",
    .msg_q = &msgq_camera,
};
static enum state_type { STATE_INIT, STATE_READY, STATE_ERROR } state;

static char* state2str(enum state_type new_state) {
  switch (new_state) {
    case STATE_INIT:
      return "STATE_INIT";
    case STATE_READY:
      return "STATE_READY";
    case STATE_ERROR:
      return "STATE_ERROR";
    default:
      return "Unknown";
  }
}

DECLARE_SET_STATE()

static bool event_handler(const struct event_header* eh) {
  struct camera_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(camera_module, camera)

  if (is_module_state_event(eh)) {
    struct module_state_event* event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  ENQUEUE_MSG(camera, CAMERA)

  return false;
}

struct camera_configuration camera_conf = {.jpeg_size = CONFIG_CAMERA_DEFAULT_JPEG_SIZE,
                                           .light_mode = CONFIG_CAMERA_DEFAULT_LIGHT_MODE,
                                           .color_saturation = CONFIG_CAMERA_DEFAULT_COLOR_SATURATION,
                                           .brightness = CONFIG_CAMERA_DEFAULT_BRIGHTNESS,
                                           .contrast = CONFIG_CAMERA_DEFAULT_CONTRAST,
                                           .special_effects = CONFIG_CAMERA_DEFAULT_SPECIAL_EFFECTS,
                                           .jpeg_quality = CONFIG_CAMERA_DEFAULT_JPEG_QUALITY,
                                           .changed = true};

static void sendImageChunck(uint32_t imageSize, uint32_t remainingSize) {
    uint32_t chunkSize = (remainingSize > CONFIG_CAMERA_IMAGE_CHUNCK_SIZE) ? CONFIG_CAMERA_IMAGE_CHUNCK_SIZE : remainingSize;
    struct camera_module_data_event* event = new_camera_module_data_event(chunkSize);
    uint32_t size = arducam_mini_2mp_fillBuffer(event->dyndata.data, chunkSize);
    event->dyndata.size = size;
    event->type = CAMERA_DATA_EVT_DATA_READY;
    event->imageSize = imageSize;
    event->isFirst = (remainingSize == imageSize);
    event->isLast = (remainingSize <= CONFIG_CAMERA_IMAGE_CHUNCK_SIZE);

    EVENT_SUBMIT(event);
}

static bool workaround_once = true;

static void takeImage(struct k_work* work) {
  LOG_INF("take image");

  if(workaround_once) {
    arducam_mini_2mp_manual_whileBalance(127,127,127);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    workaround_once = false;
  }

  uint32_t imageSize = arducam_mini_2mp_startSingleCapture();
  uint32_t remainingSize = imageSize;
  while ( remainingSize != 0) {
    sendImageChunck(imageSize, remainingSize);
    remainingSize = arducam_mini_2mp_bytesAvailable();
  }

  k_work_reschedule_for_queue(&camera_work_q, &read_camera_work, K_SECONDS(camera_interval));
  LOG_INF("take image complete");
}

static void init_module() {
  LOG_DBG("init_module");

  if (!arducam_mini_2mp_open(&camera_conf)) {
    SEND_ERROR(camera, CAMERA_EVT_ERROR, 1);
  }

  k_work_queue_init(&camera_work_q);
  k_work_queue_start(&camera_work_q, camera_work_stack_area,
                     K_THREAD_STACK_SIZEOF(camera_work_stack_area), CAMERA_WORK_THREAD_PRIORITY, NULL);

  k_work_init_delayable(&read_camera_work, takeImage);
  k_work_reschedule_for_queue(&camera_work_q, &read_camera_work,
                              K_SECONDS(CONFIG_CAMERA_DEFAULT_CAM_INTERVAL));
}

static void set_inverval_camera(uint16_t interval) {
  camera_interval = interval;
  k_work_reschedule_for_queue(&camera_work_q, &read_camera_work, K_SECONDS(0));
}


static void on_state_init(struct camera_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    init_module();
  }
}

static void on_state_ready(struct camera_msg_data* msg) {
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_JPEG_SIZE)) {
    camera_conf.jpeg_size = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_LIGHT_MODE)) {
    camera_conf.light_mode = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_COLOR_SATURATION)) {
    camera_conf.color_saturation = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_BRIGHTNESS)) {
    camera_conf.brightness = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_CONTRAST)) {
    camera_conf.contrast = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_SPECIAL_EFFECTS)) {
    camera_conf.special_effects = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_JPEG_QUALITY)) {
    camera_conf.jpeg_quality = msg->module.camera.data.val;
    camera_conf.changed = true;
  }
  if ((IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_TAKE_PICTURE)) || (IS_EVENT(msg, camera, CAMERA_EVT_TAKE_IMAGE))) {
    arducam_mini_2mp_configure_camera(&camera_conf);
    k_work_reschedule_for_queue(&camera_work_q, &read_camera_work, K_SECONDS(0));
  }
  if (IS_EVENT(msg, camera, CAMERA_EVT_SET_ARDUCAM_CAM_INTERVAL)) {
    set_inverval_camera(msg->module.camera.data.val);
  }  
}

static void on_all_states(struct camera_msg_data* msg) {}

static void module_thread_fn(void) {
  struct camera_msg_data msg;

  self.thread_id = k_current_get();

  module_start(&self);
  state_set(STATE_INIT);

  while (true) {
    module_get_next_msg(&self, &msg);
    switch (state) {
      case STATE_INIT:
        on_state_init(&msg);
        break;
      case STATE_READY:
        on_state_ready(&msg);
        break;
      default:
        LOG_WRN("Unknown state");
        break;
    }

    on_all_states(&msg);
  }
}

K_THREAD_DEFINE(camera_module_thread, CONFIG_CAMERA_THREAD_STACK_SIZE, module_thread_fn, NULL, NULL,
                NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, camera_module_event);

