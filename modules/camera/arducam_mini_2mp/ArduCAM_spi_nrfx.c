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

#include "ArduCAM.h"

#include "util_gpio.h"

#include "nrfx_spim.h"

#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(arducam_spi_nrfx, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

#define SPI_INSTANCE  2 /**< SPI instance index. */
static const nrfx_spim_t cam_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);
static volatile bool spi_busy = false;
static arducam_spi_callback_t m_spi_callback = 0;

void spim_evt_handler(nrfx_spim_evt_t const * p_event, void *p)
{
    switch(p_event->type)
    {
        case NRFX_SPIM_EVENT_DONE:
            spi_busy = false;
            if(m_spi_callback)
            {
                m_spi_callback(p_event->xfer_desc.tx_length, p_event->xfer_desc.rx_length);
            }
            break;

        default:
            LOG_ERR("unexpected spi event %d", p_event->type);
            break;
    }
}

bool arducam_spiInit_pd(const struct arducam_conf *config)
{
    init_gpio();
    IRQ_DIRECT_CONNECT(SPIM2_SPIS2_SPI2_IRQn, 0, nrfx_spim_2_irq_handler, 0);
    irq_enable(SPIM2_SPIS2_SPI2_IRQn);

    LOG_INF("arducam_spiInit_pd(sck=%d,mosi=%d,miso=%d,csn=%d)",
            config->pinSck, config->pinMosi, config->pinMiso, config->pinCsn);
    gpio_util_pin_config(config->pinCsn);

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG(
      config->pinSck, config->pinMosi, config->pinMiso, NRFX_SPIM_PIN_NOT_USED);
    spi_config.frequency = NRF_SPIM_FREQ_4M;

    nrfx_err_t ret = nrfx_spim_init(&cam_spi, &spi_config, spim_evt_handler, NULL);
    if(ret != NRFX_SUCCESS )
    {
         LOG_ERR("nrfx_spim_init failure: %d", ret);
         return false;
    }
    else
    {
         LOG_DBG("nrfx_spim_init success");
    }
    return true;
}

void arducam_spiStop_pd(const struct arducam_conf *config)
{
    nrfx_spim_uninit(&cam_spi);
}

static int retryCounter = 0;
static int maxRetires = 300;

static void waitForSpi(nrfx_err_t ret)
{
    if(ret != NRFX_SUCCESS)
    {
         LOG_ERR("nrfx_spim_xfer failure: %d", ret);
         return;
    }
    else
    {
        retryCounter = 0;
        while(spi_busy)
        {
            k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME_SHORT));
            retryCounter++;
            if(retryCounter > maxRetires)
            {
                LOG_ERR("SPI timeout");
                break;
            }
        }
    }
}

uint8_t arducam_spiWrite(uint8_t dataByte)
{
    static uint8_t return_value;
    nrfx_spim_xfer_desc_t spi_transfer;
    spi_transfer.p_tx_buffer = &dataByte;
    spi_transfer.tx_length = 1;
    spi_transfer.p_rx_buffer = &return_value;
    spi_transfer.rx_length = 1;
    LOG_DBG("t");
    spi_busy = true;
    nrfx_err_t ret = nrfx_spim_xfer(&cam_spi, &spi_transfer, 0);
    waitForSpi(ret);
    LOG_DBG("w dataByte:%d\n", dataByte);
    return return_value;
}

void arducam_spiReadMulti(uint8_t *rxBuf, uint32_t length)
{
    nrfx_spim_xfer_desc_t spi_transfer = {0};
    spi_transfer.p_rx_buffer = rxBuf;
    spi_transfer.rx_length = length;
    LOG_DBG("t");
    spi_busy = true;
    nrfx_err_t ret = nrfx_spim_xfer(&cam_spi, &spi_transfer, 0);
    waitForSpi(ret);
    LOG_DBG("w rxBuf:%d length:%d",rxBuf[0], length);
}


int arducam_bus_write(int address, int value)
{
    static uint8_t txCommand[2];
    txCommand[0] = address;
    txCommand[1] = value;
    nrfx_spim_xfer_desc_t spi_transfer;
    spi_transfer.p_tx_buffer = txCommand;
    spi_transfer.tx_length = 2;
    spi_transfer.p_rx_buffer = 0;
    spi_transfer.rx_length = 0;
    LOG_DBG("t");
    spi_busy = true;
    arducam_CS_LOW();
    nrfx_err_t ret = nrfx_spim_xfer(&cam_spi, &spi_transfer, 0);
    waitForSpi(ret);
    arducam_CS_HIGH();
    LOG_DBG("w address:%d regDat:%d\n",address, value);
    return 1;
}


uint8_t arducam_bus_read(int address)
{
    uint8_t txBuf[2] = {address, 0}, rxBuf[2];
    nrfx_spim_xfer_desc_t spi_transfer;
    spi_transfer.p_tx_buffer = txBuf;
    spi_transfer.tx_length = 2;
    spi_transfer.p_rx_buffer = rxBuf;
    spi_transfer.rx_length = 2;
    LOG_DBG("t");
    spi_busy = true;
    arducam_CS_LOW();
    nrfx_err_t ret = nrfx_spim_xfer(&cam_spi, &spi_transfer, 0);
    waitForSpi(ret);
    arducam_CS_HIGH();
    LOG_DBG("r address:%d regDat:%d\n",address, rxBuf[1]);
    return rxBuf[1];
}
