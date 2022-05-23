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

#ifndef __ARDUCAM_MINI_2MP_H
#define __ARDUCAM_MINI_2MP_H

#include <stdbool.h>
#include <stdint.h>

#include "ArduCAM.h"

typedef enum { ArducamFormatJpeg, ArducamFormatBmp } arducamFormatT;

struct camera_configuration {
  uint8_t jpeg_size;
  uint8_t light_mode;
  uint8_t color_saturation;
  uint8_t brightness;
  uint8_t contrast;
  uint8_t special_effects;
  uint8_t jpeg_quality;
  bool changed;
};

// Functions
bool arducam_mini_2mp_open(struct camera_configuration *config);
uint32_t arducam_mini_2mp_startSingleCapture(void);
uint32_t arducam_mini_2mp_bytesAvailable(void);
uint32_t arducam_mini_2mp_fillBuffer(uint8_t *buffer, uint32_t bufferSize);

void arducam_mini_2mp_configure_camera(struct camera_configuration *config);
uint32_t arducam_mini_2mp_asyncFillBuffer(uint8_t *buffer, uint32_t bufferSize);
void arducam_mini_2mp_onSpiInterrupt(uint32_t txBytes, uint32_t rxBytes);

void arducam_mini_power_down_camera();

void arducam_mini_2mp_low_reg_write(int regID, int regDat);
uint8_t arducam_mini_2mp_low_reg_read(int regID);

void arducam_mini_2mp_manual_exposure(uint32_t time_ms);
void arducam_mini_2mp_manual_gain(int gain);
void arducam_mini_2mp_manual_whileBalance(int a, int b, int c);
void arducam_mini_2mp_log_whileBalance();
void arducam_mini_2mp_set_ae_level(int level);

#endif
