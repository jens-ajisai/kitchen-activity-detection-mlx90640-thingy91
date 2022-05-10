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

#include "ArducamMini2MP.h"

#include <logging/log.h>
#include <zephyr.h>
LOG_MODULE_REGISTER(arducam_mini, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

#include "util_settings.h"


#define SPI_TRANSFER_MAX_LENGTH 240

// Private:
static uint32_t mBytesLeftInCamera = 0;

const struct arducam_conf camera_pin_conf = {.pinScl = DT_PROP(DT_NODELABEL(i2c0), scl_pin),
                                             .pinSda = DT_PROP(DT_NODELABEL(i2c0), sda_pin),
                                             .pinSck = DT_PROP(DT_NODELABEL(spi2), sck_pin),
                                             .pinMiso = DT_PROP(DT_NODELABEL(spi2), miso_pin),
                                             .pinMosi = DT_PROP(DT_NODELABEL(spi2), mosi_pin),
                                             .pinCsn = DT_GPIO_PIN(DT_NODELABEL(spi2), cs_gpios)};

static bool arducam_mini_2mp_initTransport(const struct arducam_conf *config) {
  return arducam_initTransport(config);
}

static void arducam_mini_2mp_init() { arducam_initCam(); }

static void arducam_mini_2mp_clearMemory() { arducam_clear_fifo_flag(); }

#pragma GCC diagnostic ignored "-Wunused-function" 
static void arducam_mini_2mp_stop(const struct arducam_conf *config) { arducam_stop(config); }
#pragma GCC diagnostic pop 

// TODO wrong level of abstraction!
static void arducam_mini_2mp_reset_camera() {
  LOG_INF("reset_camera");
  // Reset the CPLD
  arducam_write_reg(0x07, 0x80);
  k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
  arducam_write_reg(0x07, 0x00);
  k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
}

// TODO wrong level of abstraction!
static bool arducam_mini_2mp_check_if_camera_is_available() {
  LOG_INF("check_if_camera_is_available");

  uint8_t vid, pid;
  uint8_t temp;

  // Check if the ArduCAM SPI bus is OK
  arducam_write_reg(ARDUCHIP_TEST1, 0x55);
  temp = arducam_read_reg(ARDUCHIP_TEST1);

  if (temp != 0x55) {
    LOG_ERR("Arducam not detected. Retry");
    return false;
  } else {
    LOG_INF("Found Arducam");
  }

  // Check if the camera module type is OV2640
  arducam_wrSensorReg8_8(0xff, 0x01);
  arducam_rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  arducam_rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) || (pid != 0x42)) {
    LOG_ERR("OV2640 not detected.");
    return false;
  } else {
    LOG_INF("Found OV2640");
  }

  return true;
}

void arducam_mini_2mp_configure_camera(struct camera_configuration *config) {
  if (config->changed) {
    LOG_INF("configure_camera(%d,%d,%d,%d,%d,%d,%d)", config->jpeg_size, config->jpeg_quality,
            config->light_mode, config->color_saturation, config->brightness, config->contrast,
            config->special_effects);
    arducam_OV2640_set_JPEG_size(config->jpeg_size);
    arducam_OV2640_set_Light_Mode(config->light_mode);
    arducam_OV2640_set_Color_Saturation(config->color_saturation);
    arducam_OV2640_set_Brightness(config->brightness);
    arducam_OV2640_set_Contrast(config->contrast);
    arducam_OV2640_set_Special_effects(config->special_effects);
    arducam_OV2640_set_jpeg_quality(config->jpeg_quality);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));

// some thoughts on manual exposure, seems to be complex, so don't do.
// https://www.arducam.com/docs/camera-breakout-board/2mp-ov2640/software-guide/
//  arducam_wrSensorReg8_8(0xff, 0x01);
//  arducam_rdSensorReg8_8(0x13, 0x00); // AEC and AGC to manual
//  arducam_rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);


    config->changed = false;
  }
}

bool arducam_mini_2mp_open(struct camera_configuration *config) {
  LOG_INF("open arducam mini");
  if (!arducam_mini_2mp_initTransport(&camera_pin_conf)) return false;

  arducam_mini_2mp_reset_camera();
  if (!arducam_mini_2mp_check_if_camera_is_available()) return false;
  arducam_mini_2mp_init();

  arducam_mini_2mp_configure_camera(config);

  arducam_mini_2mp_clearMemory();

  k_sleep(K_MSEC(WAIT_AFTER_INIT));
  return true;
}

uint32_t arducam_mini_2mp_startSingleCapture() {
  LOG_INF("startSingleCapture");
  if (mBytesLeftInCamera == 0) {
    arducam_CS_LOW();
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_CS_HIGH();
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));

    arducam_flush_fifo();
    arducam_clear_fifo_flag();

    // Start capture
    arducam_start_capture();

    while (!arducam_get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
      k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    };

    mBytesLeftInCamera = arducam_read_fifo_length();
    LOG_INF("capture complete, bytes to read: %d", mBytesLeftInCamera);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));

    arducam_CS_LOW();
    arducam_set_fifo_burst();
    // https://www.arducam.com/docs/spi-cameras-for-arduino/faq/
    // I have the non-plus version. Comment this out for the plus version
    arducam_spiWrite(0);

    return mBytesLeftInCamera;
  } else {
    LOG_ERR("Last capture not read out!");
    return 0;
  }
}
uint32_t arducam_mini_2mp_bytesAvailable() { return mBytesLeftInCamera; }

uint32_t arducam_mini_2mp_fillBuffer(uint8_t *buffer, uint32_t bufferSize) {
  LOG_INF("fillBuffer(%d)", bufferSize);
  if (mBytesLeftInCamera == 0) return 0;

  uint32_t bytesToRead = (mBytesLeftInCamera > bufferSize ? bufferSize : mBytesLeftInCamera);
  mBytesLeftInCamera -= bytesToRead;
  uint32_t transactionLength;
  for (uint32_t i = 0; i < bytesToRead; i += transactionLength) {
    transactionLength =
        (bytesToRead - i) > SPI_TRANSFER_MAX_LENGTH ? SPI_TRANSFER_MAX_LENGTH : (bytesToRead - i);
    arducam_spiReadMulti(buffer, transactionLength);

    buffer += transactionLength;
  }

  if (mBytesLeftInCamera == 0) {
    arducam_CS_HIGH();
  }

  return bytesToRead;
}
