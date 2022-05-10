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

#include "nrfx_twim.h"

#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(arducam_twi_nrfx, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

#define TWIM_INSTANCE 0
static const nrfx_twim_t cam_twi = NRFX_TWIM_INSTANCE(TWIM_INSTANCE);
static volatile bool twim_busy = false;


static void twim_evt_handler(nrfx_twim_evt_t const * p_event, void *p)
{
    switch(p_event->type)
    {
        case NRFX_TWIM_EVT_DONE:
            twim_busy = false;
            break;

        case NRFX_TWIM_EVT_DATA_NACK:
        case NRFX_TWIM_EVT_ADDRESS_NACK:
            break;
        case NRFX_TWIM_EVT_OVERRUN:
          LOG_ERR("NRFX_TWIM_EVT_OVERRUN");
            break;
        case NRFX_TWIM_EVT_BUS_ERROR:
          LOG_ERR("NRFX_TWIM_EVT_BUS_ERROR");
            break;
    }

}

bool arducam_i2cInit_pd(const struct arducam_conf *config)
{
    IRQ_DIRECT_CONNECT(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn, 0, nrfx_twim_0_irq_handler, 0);
    irq_enable(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);

    nrfx_twim_config_t twi_config = NRFX_TWIM_DEFAULT_CONFIG(config->pinScl, config->pinSda);

    nrfx_err_t ret = nrfx_twim_init(&cam_twi, &twi_config, twim_evt_handler, NULL);
    if(ret != NRFX_SUCCESS )
    {
         LOG_ERR("nrfx_spim_init failure: %d", ret);
         return false;
    }
    else
    {
         LOG_DBG("nrfx_spim_init success");
    }
    nrfx_twim_enable(&cam_twi);
    return true;
}

void arducam_i2cStop_pd(const struct arducam_conf *config)
{
    nrfx_twim_disable(&cam_twi);
    nrfx_twim_uninit(&cam_twi);
}

static int retryCounter = 0;
static int maxRetires = 300;

static void waitForTwi(nrfx_err_t ret)
{
    if(ret != NRFX_SUCCESS)
    {
         LOG_ERR("nrfx_twim_xfer failure: %d", ret);
         return;
    }
    else
    {
        retryCounter = 0;
        while(twim_busy)
        {
            k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME_SHORT));
            retryCounter++;
            if(retryCounter > maxRetires)
            {
                LOG_ERR("TWI timeout");
                break;
            }
        }
    }
}

static nrfx_err_t arducam_twi_tx_rx(uint8_t *tx_data, uint32_t tx_len, uint8_t *rx_data, uint32_t rx_len)
{
//    while(twim_busy);
    nrfx_twim_xfer_desc_t twim_xfer_config;
    twim_xfer_config.address = sensor_addr;
    twim_xfer_config.p_primary_buf = tx_data;
    twim_xfer_config.primary_length = tx_len;
    twim_xfer_config.p_secondary_buf = rx_data;
    twim_xfer_config.secondary_length = rx_len;
    twim_xfer_config.type = (rx_len > 0) ? NRFX_TWIM_XFER_TXRX : NRFX_TWIM_XFER_TX;
    twim_busy = true;
    return nrfx_twim_xfer(&cam_twi, &twim_xfer_config, (rx_len > 0) ? NRFX_TWIM_FLAG_TX_NO_STOP : 0);
}

//I2C Write 8bit address, 8bit data
uint8_t arducam_wrSensorReg8_8(int regID, int regDat)
{
    static uint8_t twiBuf[2];
    twiBuf[0] = regID & 0x00FF;
    twiBuf[1] = regDat & 0x00FF;
    nrfx_err_t ret = arducam_twi_tx_rx(twiBuf, 2, 0, 0);
    waitForTwi(ret);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME_SHORT));
    return (1);
}

//I2C Read 8bit address, 8bit data
uint8_t arducam_rdSensorReg8_8(uint8_t regID, uint8_t* regDat)
{
    nrfx_err_t ret = arducam_twi_tx_rx(&regID, 1, regDat, 1);
    waitForTwi(ret);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME_SHORT));
    return (1);
}

