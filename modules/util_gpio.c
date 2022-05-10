#define MODULE util_gpio

#include "util_gpio.h"

#include <drivers/gpio.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

static const struct device* gpio_port;

struct gpio_callback gpio_cb;

void enable_interrupt(int pin) {
  if (gpio_port) {
    int rc = gpio_pin_interrupt_configure(gpio_port, pin, GPIO_INT_EDGE_BOTH);
    if (rc == -ENOTSUP) {
      LOG_ERR("Mode not supported");
    } else if (rc != 0) {
      LOG_ERR("config PIN_IN interrupt fail: %d", rc);
    }
    LOG_INF("enable interrupt of pin %d", pin);
  }
}

void disable_interrupt(int pin) {
  if (gpio_port) {
    int rc = gpio_pin_interrupt_configure(gpio_port, pin, GPIO_INT_DISABLE);
    if (rc == -ENOTSUP) {
      LOG_ERR("Mode not supported");
    } else if (rc != 0) {
      LOG_ERR("config PIN_IN interrupt fail: %d", rc);
    }
    LOG_INF("disable interrupt of pin %d", pin);
  }
}

void register_interrupt(int pin, gpio_callback_handler_t callback) {
  if (gpio_port) {
    LOG_INF("register interrupt of pin %d", pin);
    const char* gpio_dev_name = DT_PROP(DT_NODELABEL(gpio0), label);
    gpio_port = device_get_binding(gpio_dev_name);
    if (gpio_port == NULL) {
      LOG_ERR("Didn't find GPIO device %s", log_strdup(gpio_dev_name));
    }

    gpio_pin_configure(gpio_port, pin, GPIO_INPUT | GPIO_ACTIVE_LOW);

    gpio_init_callback(&gpio_cb, callback, BIT(pin));
    int rc = gpio_add_callback(gpio_port, &gpio_cb);
    if (rc == -ENOTSUP) {
      LOG_ERR("interrupts not supported");
    } else if (rc != 0) {
      LOG_ERR("set PIN_IN callback fail: %d", rc);
    }

    enable_interrupt(pin);
  }
}

void init_gpio() {
  if (gpio_port == NULL) {
    LOG_DBG("init_gpio");

    const char* gpio_dev_name = DT_PROP(DT_NODELABEL(gpio0), label);
    gpio_port = device_get_binding(gpio_dev_name);
    if (gpio_port == NULL) {
      LOG_ERR("Didn't find GPIO device %s", log_strdup(gpio_dev_name));
    }
  }
}

void gpio_util_pin_config(gpio_pin_t pin) {
  if (gpio_port) {
    gpio_pin_configure(gpio_port, pin, GPIO_OUTPUT_HIGH | GPIO_ACTIVE_HIGH);
  }
}

void gpio_util_pin_low(gpio_pin_t pin) {
  if (gpio_port) {
    gpio_pin_set_raw(gpio_port, pin, 0);
  }
}

void gpio_util_pin_high(gpio_pin_t pin) {
  if (gpio_port) {
    gpio_pin_set_raw(gpio_port, pin, 1);
  }
}
