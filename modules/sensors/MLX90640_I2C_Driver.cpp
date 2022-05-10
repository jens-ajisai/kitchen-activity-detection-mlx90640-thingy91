/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * !!! Modifications applied to work with the Zephyr API as the original code was written for mbed envornment
 *
 */
#include "MLX90640_I2C_Driver.h"

#include <device.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <zephyr.h>
LOG_MODULE_REGISTER(sensor_mlx60940_i2c, CONFIG_SENSORS_MODULE_LOG_LEVEL);

/* I2C device. */
static const struct device* i2c_dev;

int MLX90640_I2CInit() {
  i2c_dev = device_get_binding(DT_LABEL(DT_NODELABEL(i2c0)));
  if (i2c_dev == NULL) {
    /* No valid device found */
    return -1;
  }
  return 0;
}

int MLX90640_I2CGeneralReset(void) {
  int ack;
  uint8_t cmd[2] = {0, 0};

  cmd[0] = 0x00;
  cmd[1] = 0x06;

  LOG_DBG("MLX90640_I2CGeneralReset i2c_write(i2c_dev, %d, %d, %d)", (uint8_t)cmd[1], 1, cmd[0]);
  ack = i2c_write(i2c_dev, &cmd[1], 1, cmd[0]);

  if (ack != 0x00) {
    return -1;
  }

  return 0;
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead,
                     uint16_t* data) {
  int ack = 0;
  int cnt = 0;
  int i = 0;
  uint8_t cmd[2] = {0, 0};
  uint8_t i2cData[1664] = {0};

  cmd[0] = startAddress >> 8;
  cmd[1] = startAddress & 0x00FF;

  LOG_DBG("3MLX90640_I2CRead i2c_write_read(i2c_dev, %d, %d, %d, i2cData, %d)", slaveAddr,
          (uint16_t)cmd[0], 2, 2 * nMemAddressRead);
  ack = i2c_write_read(i2c_dev, slaveAddr, cmd, 2, i2cData, 2 * nMemAddressRead);

  if (ack != 0x00) {
    return -1;
  }

  for (cnt = 0; cnt < nMemAddressRead; cnt++) {
    i = cnt << 1;
    *data++ = (uint16_t)i2cData[i] * 256 + (uint16_t)i2cData[i + 1];
  }

  return 0;
}

int MLX90640_I2CFreqSet(int freq) {
  uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
  switch (freq) {
    case I2C_SPEED_STANDARD:
      i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
      break;
    case I2C_SPEED_FAST:
      i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;
      break;
    default:
      return -1;
      break;
  }
  return i2c_configure(i2c_dev, i2c_cfg);
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
  int ack = 0;
  uint8_t cmd[4] = {0, 0, 0, 0};
  static uint16_t dataCheck;

  cmd[0] = writeAddress >> 8;
  cmd[1] = writeAddress & 0x00FF;
  cmd[2] = data >> 8;
  cmd[3] = data & 0x00FF;

  LOG_DBG("MLX90640_I2CWrite i2c_write(i2c_dev, %d, %d, %d)", (uint32_t)cmd[0], 4, slaveAddr);
  ack = i2c_write(i2c_dev, cmd, 4, slaveAddr);

  if (ack != 0x00) {
    return -1;
  }

  MLX90640_I2CRead(slaveAddr, writeAddress, 1, &dataCheck);

  if (dataCheck != data) {
    return -2;
  }

  return 0;
}
