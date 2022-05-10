/*
 * This code is based on the nrf_desktop which has the below license
 * nrf/applications/nrf_desktop/configuration
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/gpio_pins.h>

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matrix.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin col[] = {};

// Document .port
// for the nrf52840 maybe it is the Boot button P1.13?
static const struct gpio_pin row[] = {
	{ .port = 0, .pin = DT_GPIO_PIN(DT_NODELABEL(button0), gpios) },
};
