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

#ifndef __ArduCAM_H
#define __ArduCAM_H

#include "ArduCAM_regs.h"
#include <stdbool.h>

#define pgm_read_word(x)        ( ((*((unsigned char *)x + 1)) << 8) + (*((unsigned char *)x)))

struct arducam_conf
{
    uint32_t pinCsn;
    uint32_t pinMosi;
    uint32_t pinMiso;
    uint32_t pinSck;
    uint32_t pinSda;
    uint32_t pinScl;
};

/****************************************************************/
/* define a structure for sensor register initialization values */
/****************************************************************/
typedef void (*arducam_spi_callback_t)(uint32_t tx_bytes, uint32_t rx_bytes);

#define WAIT_FOR_CAMERA_TIME_SHORT 1
#define WAIT_FOR_CAMERA_TIME 100
#define WAIT_AFTER_INIT 3000

void arducam_CS_LOW(void);
void arducam_CS_HIGH(void);

bool arducam_initTransport(const struct arducam_conf *config);
void arducam_stop(const struct arducam_conf *config);
void arducam_initCam();
bool arducam_i2cInit(const struct arducam_conf *config);
bool arducam_i2cInit_pd(const struct arducam_conf *config);
void arducam_i2cStop(const struct arducam_conf *config);
void arducam_i2cStop_pd(const struct arducam_conf *config);

bool arducam_spiInit(const struct arducam_conf *config);
bool arducam_spiInit_pd(const struct arducam_conf *config);
void arducam_spiStop(const struct arducam_conf *config);
void arducam_spiStop_pd(const struct arducam_conf *config);

int arducam_bus_write(int address, int value);
uint8_t arducam_bus_read(int address);

uint8_t arducam_spiWrite(uint8_t dataByte);
void arducam_spiReadMulti(uint8_t *rxBuf, uint32_t length);

void arducam_flush_fifo(void);
void arducam_start_capture(void);
void arducam_clear_fifo_flag(void);
uint8_t arducam_read_fifo(void);

uint8_t arducam_read_reg(uint8_t addr);
void arducam_write_reg(uint8_t addr, uint8_t data);

uint32_t arducam_read_fifo_length(void);
void arducam_set_fifo_burst(void);
void arducam_set_bit(uint8_t addr, uint8_t bit);
void arducam_clear_bit(uint8_t addr, uint8_t bit);
uint8_t arducam_get_bit(uint8_t addr, uint8_t bit);

uint8_t arducam_rdSensorReg8_8(uint8_t regID, uint8_t* regDat);
uint8_t arducam_wrSensorReg8_8(int regID, int regDat);
int arducam_wrSensorRegs8_8(const struct sensor_reg*);

/*
int arducam_wrSensorRegs8_16(const struct sensor_reg*);
int arducam_wrSensorRegs16_8(const struct sensor_reg*);
int arducam_wrSensorRegs16_16(const struct sensor_reg*);
int arducam_wrSensorRegs(const struct sensor_reg*);

uint8_t arducam_wrSensorReg(int regID, int regDat);
uint8_t arducam_wrSensorReg8_16(int regID, int regDat);
uint8_t arducam_wrSensorReg16_8(int regID, int regDat);
uint8_t arducam_wrSensorReg16_16(int regID, int regDat);

uint8_t arducam_rdSensorReg16_8(uint16_t regID, uint8_t* regDat);
uint8_t arducam_rdSensorReg8_16(uint8_t regID, uint16_t* regDat);
uint8_t arducam_rdSensorReg16_16(uint16_t regID, uint16_t* regDat);
*/

void arducam_OV2640_set_JPEG_size(uint8_t size);
void arducam_OV2640_set_jpeg_quality(uint8_t value);
void arducam_OV2640_set_Light_Mode(uint8_t Light_Mode);
void arducam_OV2640_set_Color_Saturation(uint8_t Color_Saturation);
void arducam_OV2640_set_Contrast(uint8_t Contrast);
void arducam_OV2640_set_Special_effects(uint8_t Special_effect);
void arducam_OV2640_set_Brightness(uint8_t Brightness);

#endif
