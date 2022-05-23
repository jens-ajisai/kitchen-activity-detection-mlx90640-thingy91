#define MODULE main

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <dfu/mcuboot.h>
#endif

#ifdef CONFIG_MCUMGR_CMD_SHELL_MGMT
#include "shell_mgmt/shell_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#ifdef CONFIG_NRF_MODEM_LIB
#include "common/diag.h"
#include <memfault/http/http_client.h>
#include <memfault/metrics/metrics.h>
#include "memfault/ports/zephyr/http.h"
#endif
#endif

#ifdef CONFIG_NRF_MODEM_LIB
#include <modem/lte_lc.h>
#endif

#include <caf/events/module_state_event.h>
#include <event_manager.h>
#include <logging/log.h>
#include <zephyr.h>

#include "common/modules_common.h"
#include "utils.h"
#include "watchdog/watchdog.h"

#ifdef CONFIG_USB_DEVICE_STACK
#include "usb_console.h"
#endif

LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

void sendReady() {
    module_set_state(MODULE_STATE_READY);
}

#ifdef CONFIG_MODEM_KEY_MGMT
#include "adafruit_certificates.h"
#include "mqtt/mqtt_client.h"
#include "https_client.h"

static sec_tag_t adafruit_tls_sec_tag[] = {kAdafruitRootCert_DigicertRootCa,
                                           kAdafruitRootCert_DigicertGeotrustCa,
                                           kAdafruitRootCert_Adafruit};

static const char* adafruit_certs[] = {ADAFRUIT_CERTS_PEM};
#endif

#ifdef CONFIG_NRF_MODEM_LIB
static int connect_lte() {
  LOG_DBG("Waiting for network.. ");
  int err = lte_lc_init_and_connect();
  if (err) {
    LOG_ERR("Failed to connect to the LTE network, err %d", err);
    return 1;
  }
  send_led_event(LED_EVENT_CONNECTED);
  return 0;
}
#endif

void main(void) {
#ifdef CONFIG_USB_DEVICE_STACK
  enable_usb_console();
#endif

#ifdef CONFIG_BOOTLOADER_MCUBOOT
  /* Check if the image is run in the REVERT mode and eventually
   * confirm it to prevent reverting on the next boot.
   */
  if (mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT) {
    if (boot_write_img_confirmed()) {
      LOG_ERR(
          "Confirming firmware image failed, it will be reverted on the next "
          "boot.");
    } else {
      LOG_INF("New device firmware image confirmed.");
    }
  }
#endif

#ifdef CONFIG_MCUMGR_CMD_SHELL_MGMT
  shell_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
  os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
  img_mgmt_register_group();
#endif

  LOG_INF("Hello World! %s\n", CONFIG_BOARD);
  LOG_INF("build time: " __DATE__ " " __TIME__);

  init_utils();

#ifdef CONFIG_CAF_LEDS
  send_led_event(LED_EVENT_STARTUP);
#endif

#ifdef CONFIG_MODEM_KEY_MGMT
  cert_provision(adafruit_tls_sec_tag, adafruit_certs,
                 sizeof(adafruit_tls_sec_tag) / sizeof(adafruit_tls_sec_tag[0]));
#endif

#ifdef CONFIG_NRF_MODEM_LIB
  connect_lte();
#endif

#ifdef CONFIG_MEMFAULT
#ifdef CONFIG_NRF_MODEM_LIB
  memfault_zephyr_port_post_data();
#endif
#endif

  if (event_manager_init()) {
    LOG_ERR("Event manager not initialized");

  } else {
#ifndef CONFIG_WAIT_FOR_MANUAL_START
    sendReady();
#endif
  }
#ifdef CONFIG_WATCHDOG
  init_and_start_watchdog();
#endif
}
