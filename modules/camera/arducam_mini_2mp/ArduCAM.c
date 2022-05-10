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

#include <logging/log.h>
LOG_MODULE_REGISTER(arducam, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

bool arducam_initTransport(const struct arducam_conf *config)
{
    LOG_INF("arducam_initTransport");

    bool success = arducam_i2cInit(config);
    return (success && arducam_spiInit(config));
}

void arducam_stop(const struct arducam_conf *config)
{
    LOG_INF("arducam_initTransport");

    arducam_i2cStop(config);
    arducam_spiStop(config);
}
