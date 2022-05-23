/*
 * Heavily based on the Arduino driver library for Arducam, the image transfer demo.
 * Modified a lot and removed everything I do not use.
 * https://github.com/ArduCAM/Arduino
 * https://github.com/ArduCAM/nrf52-ble-image-transfer-demo
 *
 * ArduCAM.cpp - Arduino library support for CMOS Image Sensor
 * Copyright (C)2011-2015 ArduCAM.com. All right reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * ...
 * 
 * Also some functions about manual exposure, gain and white balance are taken from
 * https://github.com/espressif/esp32-camera¥
 * Apache-2.0 license
 */


#include "ArducamMini2MP.h"

#include <logging/log.h>
#include <zephyr.h>
#include <math.h>
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
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_JPEG_size(config->jpeg_size);
    arducam_OV2640_set_Light_Mode(config->light_mode);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_Color_Saturation(config->color_saturation);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_Brightness(config->brightness);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_Contrast(config->contrast);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_Special_effects(config->special_effects);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));
    arducam_OV2640_set_jpeg_quality(config->jpeg_quality);
    k_sleep(K_MSEC(WAIT_FOR_CAMERA_TIME));

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

  arducam_mini_2mp_log_whileBalance();
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

void arducam_mini_2mp_low_reg_write(int regID, int regDat) {
  arducam_wrSensorReg8_8(regID, regDat);
}

uint8_t arducam_mini_2mp_low_reg_read(int regID) {
  uint8_t regDat;
  arducam_rdSensorReg8_8(regID, &regDat);
  return regDat;
}

void arducam_mini_2mp_enable_aec_agc(bool enable) {
  uint8_t temp;
  arducam_wrSensorReg8_8(0xff, 0x01);
  arducam_rdSensorReg8_8(0x13, &temp);
  if (enable) {
    temp |= (1 << 0);  // Enable automatic exposure
    temp |= (1 << 2);  // Enable AGC
    arducam_wrSensorReg8_8(0x13, temp);
  } else {
    temp &= ~(1 << 0);  // Close automatic exposure
    temp &= ~(1 << 2);  // Close AGC
  }
  arducam_wrSensorReg8_8(0xff, 0x01);
}

#define IMAGE_WIDTH 1600
#define IMAGE_HEIGHT 1200
#define HORIZONTAL_CLOCK_BLANKING 322
#define VERTICAL_LINE_BLANKING 48
#define CAMERA_CLOCK 36


void arducam_mini_2mp_manual_exposure(uint32_t time_ms) {
  // expect 1600 x 1200
  // modifications of 1922 and 41.66 required for other resolutions
  // no idea where 41.66 come from. Expected 53.39.
  uint32_t tex = time_ms * 1000000;
  uint16_t AEC = tex / ((IMAGE_WIDTH + HORIZONTAL_CLOCK_BLANKING) * 41.66);

  AEC = MIN(AEC, (IMAGE_HEIGHT + VERTICAL_LINE_BLANKING));

  LOG_INF("set AEC value %d", AEC);

  if (AEC) {
    char exposure1 = AEC & 0x3;
    char exposure2 = (AEC >> 2) & 0xff;
    char exposure3 = (AEC >> 10) & 0x3f;

    arducam_mini_2mp_enable_aec_agc(false);
    uint8_t temp;
    arducam_wrSensorReg8_8(0xff, 0x01);
    arducam_rdSensorReg8_8(0x04, &temp);
    temp = temp & 0XFC;
    arducam_wrSensorReg8_8(0x04, temp | exposure1);
    arducam_rdSensorReg8_8(0x10, &temp);
    temp = temp & 0;
    arducam_wrSensorReg8_8(0x10, temp | exposure2);
    arducam_rdSensorReg8_8(0x45, &temp);
    temp = temp & 0XE0;
    arducam_wrSensorReg8_8(0x45, temp | exposure3);
  } else {
    // set auto exposure, auto gain
    arducam_mini_2mp_enable_aec_agc(true);
  }
}

#define NUM_GAIN_LEVELS (31)
const uint8_t agc_gain_tbl[NUM_GAIN_LEVELS + 1] = {
    0X45, 0x00, 0x10, 0x18, 0x30, 0x34, 0x38, 0x3C, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF};

void arducam_mini_2mp_manual_gain(int gain) {
  gain = MAX(0, gain);
  gain = MIN(gain, NUM_GAIN_LEVELS);

  LOG_INF("set gain value %d", gain);

  if (gain) {
    arducam_mini_2mp_enable_aec_agc(false);
    arducam_wrSensorReg8_8(0xff, 0x01);
    arducam_wrSensorReg8_8(agc_gain_tbl[0], agc_gain_tbl[gain]);
  } else {
    // set auto gain
    arducam_mini_2mp_enable_aec_agc(true);
  }
}

#define NUM_WB_MODES (4)
static const uint8_t wb_modes_regs[NUM_WB_MODES + 1][3] = {
    {0XCC, 0XCD, 0XCE}, {0x5E, 0X41, 0x54}, /* sunny */
    {0x65, 0X41, 0x4F},                     /* cloudy */
    {0x52, 0X41, 0x66},                     /* office */
    {0x42, 0X3F, 0x71},                     /* home */
};

void arducam_mini_2mp_manual_whileBalance(int a, int b, int c) {
  a = MAX(0, a);
  a = MIN(a, 254);
  b = MAX(0, b);
  b = MIN(b, 254);
  c = MAX(0, c);
  c = MIN(c, 254);

  LOG_INF("set mode values %d, %d, %d", a, b, c);

  if (a && b && c) {
    arducam_wrSensorReg8_8(0xff, 0x00);
    arducam_wrSensorReg8_8(0xC7, 0x40);
    arducam_wrSensorReg8_8(wb_modes_regs[0][0], a);
    arducam_wrSensorReg8_8(wb_modes_regs[0][1], b);
    arducam_wrSensorReg8_8(wb_modes_regs[0][2], c);
  } else {
    // set auto white balance
    arducam_wrSensorReg8_8(0xff, 0x00);
    arducam_wrSensorReg8_8(0xC7, 0x00);
  }
};

void arducam_mini_2mp_log_whileBalance() {
  uint8_t awb[3];
  for (int i = 0; i < 3; i++) {
    arducam_rdSensorReg8_8(wb_modes_regs[0][i], &awb[i]);
  }
  LOG_INF("AWB: [%d,%d,%d]", awb[0], awb[1], awb[2]);
}

#define AEW 0x24
#define AEB 0x25
#define VV 0x26
#define NUM_AE_LEVELS (5)
static const uint8_t ae_levels_regs[NUM_AE_LEVELS + 1][3] = {
    {AEW, AEB, VV},     {0x20, 0X18, 0x60}, {0x34, 0X1C, 0x00},
    {0x3E, 0X38, 0x81}, {0x48, 0X40, 0x81}, {0x58, 0X50, 0x92},
};

void arducam_mini_2mp_set_ae_level(int level) {
  level += 3;

  level = MAX(1, level);
  level = MIN(level, NUM_AE_LEVELS);

  LOG_INF("set ae level value %d", level);

  arducam_wrSensorReg8_8(0xff, 0x00);
  for (int i = 0; i < 3; i++) {
    arducam_wrSensorReg8_8(ae_levels_regs[0][i], ae_levels_regs[level][i]);
  }
};
