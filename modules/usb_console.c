#include "utils.h"

#include <drivers/uart.h>
#include <usb/usb_device.h>

#if defined(CONFIG_WAIT_FOR_SERIAL_ATTACHED)
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
             "Console device is not ACM CDC UART device");
BUILD_ASSERT(CONFIG_UART_LINE_CTRL == 1);
#endif

#define USB_SERIALNUMBER_TEMPLATE "HTY_%04X%08X"

static uint8_t usb_serial_str[] = "HTY_12PLACEHLDRS";

/* Overriding weak function to set iSerialNumber at runtime. */
uint8_t *usb_update_sn_string_descriptor(void) {
  snprintk(usb_serial_str, sizeof(usb_serial_str), USB_SERIALNUMBER_TEMPLATE,
           (uint32_t)(NRF_FICR->DEVICEADDR[1] & 0x0000FFFF) | 0x0000C000,
           (uint32_t)NRF_FICR->DEVICEADDR[0]);

  return usb_serial_str;
}

void enable_usb_console() {
  if (usb_enable(NULL)) {
    return;
  }

#if defined(CONFIG_WAIT_FOR_SERIAL_ATTACHED)
  const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
  uint32_t dtr = 0;

  /* Poll if the DTR flag was set */
  while (!dtr) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    /* Give CPU resources to low priority threads. */
    k_sleep(K_MSEC(100));
	// CAF LED module not yet available at this point of time
    blink(100);
  }
#endif
}
