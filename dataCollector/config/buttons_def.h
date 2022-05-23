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
const struct {
} buttons_def_include_once;

static const struct gpio_pin col[] = {};

#define DT_PORT_BY_PH(phandle) DT_CAT(phandle, _P_port)

#define DT_GPIO_PORT(node_id, gpio_pha) \
	DT_PORT_BY_PH(DT_PHANDLE_BY_IDX(node_id, gpio_pha, 0))

// Document .port
// for the nrf52840 maybe it is the Boot button P1.13?
static const struct gpio_pin row[] = {
    {.port = DT_GPIO_PORT(DT_NODELABEL(button0), gpios),
     .pin = DT_GPIO_PIN(DT_NODELABEL(button0), gpios)},
};
