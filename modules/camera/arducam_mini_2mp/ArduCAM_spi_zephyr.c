/*
 * Heavily based on the Arduino driver library for Arducam. 
 * Modified a lot and removed everything I do not use.
 * https://github.com/ArduCAM/Arduino
 * 
 * ArduCAM.cpp - Arduino library support for CMOS Image Sensor
 * Copyright (C)2011-2015 ArduCAM.com. All right reserved
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * ...
*/

#include <drivers/spi.h>
#include <logging/log.h>
#include <zephyr.h>

#include "ArduCAM.h"
#include "util_gpio.h"
LOG_MODULE_REGISTER(arducam_spi_zephyr, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

static const struct device *spi_dev;

static struct spi_cs_control spi_cs = {
    .gpio_dev = NULL, .gpio_pin = 0, .gpio_dt_flags = 0, .delay = 10};

// Arducam requires either SPI_HOLD_ON_CS with manual CS pin handling added or SPI_LOCK_ON
static const struct spi_config spi_cfg = {.frequency = MHZ(4),  // NRF_SPIM_FREQ_4M,
                                          .operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER |
                                                       SPI_TRANSFER_MSB | SPI_LINES_SINGLE |
                                                       SPI_HOLD_ON_CS,
                                          .slave = 0,
                                          .cs = &spi_cs};

#define ARDUCAM_SPI DT_BUS(DT_NODELABEL(arducam_spi0))

bool arducam_spiInit_pd(const struct arducam_conf *conf) {
  ARG_UNUSED(conf);

  LOG_INF("arducam_spiInit");
  spi_dev = device_get_binding(DT_LABEL(ARDUCAM_SPI));
  if (!spi_dev) {
    LOG_ERR("Cannot find SPI device %s!", DT_LABEL(ARDUCAM_SPI));
    return false;
  }
  spi_cs.gpio_dev = device_get_binding(DT_GPIO_LABEL(ARDUCAM_SPI, cs_gpios));
  spi_cs.gpio_pin = DT_GPIO_PIN(ARDUCAM_SPI, cs_gpios);
  spi_cs.gpio_dt_flags = DT_GPIO_FLAGS(ARDUCAM_SPI, cs_gpios);

  if (!device_is_ready(spi_dev)) {
    LOG_ERR("SPI spi_dev %s not ready", spi_dev->name);
    return false;
  }
  return true;
}

uint8_t arducam_spiWrite(uint8_t dataByte) {
  uint8_t return_value;

  const struct spi_buf tx_buf = {.buf = &dataByte, .len = 1};
  const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

  const struct spi_buf rx_buf = {.buf = &return_value, .len = 1};
  const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

  spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
  return return_value;
}

void arducam_spiReadMulti(uint8_t *rxBuf, uint32_t length) {
  const struct spi_buf rx_buf = {.buf = rxBuf, .len = length};
  const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

  spi_read(spi_dev, &spi_cfg, &rx);
}

int arducam_bus_write(int address, int value) {
  uint8_t txBuf[2] = {address, value};

  const struct spi_buf tx_buf = {.buf = txBuf, .len = 2};
  const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

  arducam_CS_LOW();
  spi_write(spi_dev, &spi_cfg, &tx);
  arducam_CS_HIGH();

  LOG_DBG("w address:%d regDat:%d", address, value);
  LOG_DBG("w txBuf: %d %d", txBuf[0], txBuf[1]);
  return 1;
}

uint8_t arducam_bus_read(int address) {
  uint8_t txBuf[2] = {address, 0}, rxBuf[2] = {0, 0};

  const struct spi_buf tx_buf = {.buf = txBuf, .len = 2};
  const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

  const struct spi_buf rx_buf = {.buf = rxBuf, .len = 2};
  const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

  arducam_CS_LOW();
  spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
  arducam_CS_HIGH();
  LOG_DBG("s address:%d recv:%d, %d", address, txBuf[0], txBuf[1]);
  LOG_DBG("r address:%d recv:%d, %d", address, rxBuf[0], rxBuf[1]);
  return rxBuf[1];
}
