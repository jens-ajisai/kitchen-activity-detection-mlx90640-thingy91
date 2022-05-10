#define MODULE main

#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <zephyr.h>

#include "common/modules_common.h"
#include "utils.h"
#include "util_settings.h"

#ifdef CONFIG_USB_DEVICE_STACK
#include "usb_console.h"
#endif

LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

void sendReady() {
    module_set_state(MODULE_STATE_READY);
}

void main(void) {
  // Reminder: CAF_LED only works after MODULE_STATE_READY!
  blink(1000);
#ifdef CONFIG_USB_DEVICE_STACK
  enable_usb_console();
#endif

  LOG_INF("Hello World! %s", CONFIG_BOARD);
  LOG_INF("build time: " __DATE__ " " __TIME__);

  if (event_manager_init()) {
    LOG_ERR("Event manager not initialized");
  } else {
#ifndef CONFIG_WAIT_FOR_MANUAL_START
    // sometimes crashes if not waiting here ... no idea why
    k_sleep(K_SECONDS(1));
    settings_util_init();
    sendReady();
#endif
  }

#ifdef CONFIG_CAF_LEDS
  send_led_event(LED_EVENT_STARTUP);
#endif

}
