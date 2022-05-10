#ifndef __UTIL_GPIO_H
#define __UTIL_GPIO_H

#include <drivers/gpio.h>

void init_gpio();
void register_interrupt(int pin, gpio_callback_handler_t callback);
void enable_interrupt(int pin);
void disable_interrupt(int pin);
void gpio_util_power_down();
void gpio_util_power_up();

void gpio_util_pin_config(gpio_pin_t pin);
void gpio_util_pin_low(gpio_pin_t pin);
void gpio_util_pin_high(gpio_pin_t pin);
#endif
