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

#include <drivers/i2c.h>

#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(arducam_i2c_zephyr, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

static const struct device *i2c_dev;

#define ARDUCAM_I2C DT_BUS(DT_NODELABEL(arducam_i2c0))

bool arducam_i2cInit_pd(const struct arducam_conf *config)
{
  i2c_dev = device_get_binding(DT_LABEL(ARDUCAM_I2C));
  if (i2c_dev == NULL) {
      LOG_ERR("Cannot find I2C device %s!", DT_LABEL(ARDUCAM_I2C));
      return false;
    }
    return true;
}

//I2C Write 8bit address, 8bit data
uint8_t arducam_wrSensorReg8_8(int regID, int regDat)
{
    struct i2c_msg msg;
    uint8_t txBuf[2] = {regID & 0x00FF, regDat & 0x00FF};

    msg.buf = txBuf;
    msg.flags = 0;
    msg.len = sizeof(txBuf);

    int ret = i2c_transfer(i2c_dev, &msg, 1, sensor_addr);
    if(ret) LOG_ERR("i2c_transfer failure: %d", ret);

    LOG_DBG("w regID:%d regDat:%d\n",regID, regDat);
    return 0;
}

//I2C Read 8bit address, 8bit data
uint8_t arducam_rdSensorReg8_8(uint8_t regID, uint8_t* regDat)
{
    int ret = i2c_write_read(i2c_dev, sensor_addr, &regID, 1, regDat, 1);
    if(ret) LOG_ERR("i2c_transfer failure: %d", ret);
    LOG_DBG("r regID:%d regDat:%d\n",regID, *regDat);
    return 0;
}

/*
//I2C Write 8bit address, 16bit data
uint8_t arducam_wrSensorReg8_16(int regID, int regDat)
{
    struct i2c_msg msg;
    uint8_t txBuf[3] = {regID & 0x00FF, regDat >> 8, regDat & 0x00FF};

    msg.buf = txBuf;
    msg.flags = 0;
    msg.len = sizeof(txBuf);

    return i2c_transfer(i2c_dev, &msg, 1, sensor_addr);
}

//I2C Read 8bit address, 16bit data
uint8_t arducam_rdSensorReg8_16(uint8_t regID, uint16_t* regDat)
{
    return i2c_write_read(i2c_dev, sensor_addr, &regID, 1, regDat, 2);
}

//I2C Write 16bit address, 8bit data
uint8_t arducam_wrSensorReg16_8(int regID, int regDat)
{
    struct i2c_msg msg;
    uint8_t txBuf[3] = {regID >> 8, regID & 0x00FF, regDat & 0x00FF};

    msg.buf = txBuf;
    msg.flags = 0;
    msg.len = sizeof(txBuf);

    return i2c_transfer(i2c_dev, &msg, 1, sensor_addr);
}

//I2C Read 16bit address, 8bit data
uint8_t arducam_rdSensorReg16_8(uint16_t regID, uint8_t* regDat)
{
    uint8_t txBuf[2] = {regID >> 8, regID & 0x00FF};
    return i2c_write_read(i2c_dev, sensor_addr, txBuf, 2, regDat, 1);
}

//I2C Write 16bit address, 16bit data
uint8_t arducam_wrSensorReg16_16(int regID, int regDat)
{
    struct i2c_msg msg;
    uint8_t txBuf[4] = {regID >> 8, regID & 0x00FF, regDat >> 8, regDat & 0x00FF};

    msg.buf = txBuf;
    msg.flags = 0;
    msg.len = sizeof(txBuf);

    return i2c_transfer(i2c_dev, &msg, 1, sensor_addr);
}

//I2C Read 16bit address, 16bit data
uint8_t arducam_rdSensorReg16_16(uint16_t regID, uint16_t* regDat)
{
    uint8_t txBuf[2] = {regID >> 8, regID & 0x00FF};
    return i2c_write_read(i2c_dev, sensor_addr, txBuf, 2, regDat, 2);
}



//I2C Array Write 8bit address, 16bit data
int arducam_wrSensorRegs8_16(const struct sensor_reg reglist[])
{
  unsigned int reg_addr =0, reg_val = 0;
  const struct sensor_reg *next = reglist;

  while ((reg_addr != 0xff) | (reg_val != 0xffff))
  {
    reg_addr = pgm_read_word(&next->reg);
    reg_val = pgm_read_word(&next->val);
    arducam_wrSensorReg8_16(reg_addr, reg_val);
    next++;
  }

  return 1;
}

//I2C Array Write 16bit address, 8bit data
int arducam_wrSensorRegs16_8(const struct sensor_reg reglist[])
{
  unsigned int reg_addr = 0;
  unsigned char reg_val = 0;
  const struct sensor_reg *next = reglist;

  while ((reg_addr != 0xffff) | (reg_val != 0xff))
  {
    reg_addr = pgm_read_word(&next->reg);
    reg_val = pgm_read_word(&next->val);
    arducam_wrSensorReg16_8(reg_addr, reg_val);
    next++;
  }

  return 1;
}

//I2C Array Write 16bit address, 16bit data
int arducam_wrSensorRegs16_16(const struct sensor_reg reglist[])
{

  unsigned int reg_addr, reg_val;
  const struct sensor_reg *next = reglist;
  reg_addr = pgm_read_word(&next->reg);
  reg_val = pgm_read_word(&next->val);
  while ((reg_addr != 0xffff) | (reg_val != 0xffff))
  {
    arducam_wrSensorReg16_16(reg_addr, reg_val);
    next++;
    reg_addr = pgm_read_word(&next->reg);
    reg_val = pgm_read_word(&next->val);
  }

  return 1;
}
*/

