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

#ifndef __ArduCAM_REGS_H
#define __ArduCAM_REGS_H

#include <stdint.h>

/****************************************************/
/* I2C Control Definition 							*/
/****************************************************/
#define I2C_ADDR_8BIT  0
#define I2C_ADDR_16BIT 1
#define I2C_REG_8BIT   0
#define I2C_REG_16BIT  1
#define I2C_DAT_8BIT   0
#define I2C_DAT_16BIT  1

/* Register initialization tables for SENSORs */
/* Terminating list entry for reg */
#define SENSOR_REG_TERM_8BIT 	0xFF
#define SENSOR_REG_TERM_16BIT	0xFFFF
/* Terminating list entry for val */
#define SENSOR_VAL_TERM_8BIT	0xFF
#define SENSOR_VAL_TERM_16BIT	0xFFFF

//Define maximum frame buffer size
#define MAX_FIFO_SIZE			0x5FFFF	// 384KByte


/****************************************************/
/* ArduChip registers definition 					*/
/****************************************************/
#define RWBIT					0x80  //READ AND WRITE BIT IS BIT[7]

#define ARDUCHIP_TEST1       	0x00  //TEST register

#define ARDUCHIP_MODE      		0x02  //Mode register
#define MCU2LCD_MODE       		0x00
#define CAM2LCD_MODE       		0x01
#define LCD2MCU_MODE       		0x02

#define ARDUCHIP_TIM       		0x03  //Timming control

//#define FIFO_PWRDN_MASK	   	0x20  	//0 = Normal operation, 1 = FIFO power down
//#define LOW_POWER_MODE	    0x40  	//0 = Normal mode, 		1 = Low power mode

#define ARDUCHIP_FIFO      		0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK    		0x01
#define FIFO_START_MASK    		0x02
#define FIFO_RDPTR_RST_MASK     0x10
#define FIFO_WRPTR_RST_MASK     0x20

#define ARDUCHIP_GPIO			0x06  //GPIO Write Register
#define GPIO_RESET_MASK			0x01  //0 = Sensor reset, 1 = Sensor normal operation
#define GPIO_PWDN_MASK			0x02  //0 = Sensor normal operation, 1 = Sensor standby
#define GPIO_PWREN_MASK			0x04  //0 = Sensor LDO disable, 	 1 = sensor LDO enable

#define BURST_FIFO_READ			0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ		0x3D  //Single FIFO read operation

#define ARDUCHIP_REV       		0x40  //ArduCHIP revision
#define VER_LOW_MASK       		0x3F
#define VER_HIGH_MASK      		0xC0

#define ARDUCHIP_TRIG      		0x41  //Trigger source
#define VSYNC_MASK         		0x01
#define SHUTTER_MASK       		0x02
#define CAP_DONE_MASK      		0x08

#define FIFO_SIZE1				0x42  //Camera write FIFO size[7:0] for burst to read
#define FIFO_SIZE2				0x43  //Camera write FIFO size[15:8]
#define FIFO_SIZE3				0x44  //Camera write FIFO size[18:16]

#define sensor_addr (0x60 >> 1)

/****************************************************/
/* Sensor related definition 						*/
/****************************************************/
#define BMP 	0
#define JPEG	1

#define OV2640  	5

#define OV2640_CHIPID_HIGH 	0x0A
#define OV2640_CHIPID_LOW 	0x0B

#define OV2640_160x120 		 0
#define OV2640_176x144 		 1
#define OV2640_320x240 		 2
#define OV2640_352x288 		 3
#define OV2640_640x480		 4
#define OV2640_800x600 		 5
#define OV2640_1024x768		 6
#define OV2640_1280x1024	 7
#define OV2640_1600x1200	 8

// Light Mode
#define Auto                 0
#define Sunny                1
#define Cloudy               2
#define Office               3
#define Home                 4

// Color Saturation
#define Saturation4          0
#define Saturation3          1
#define Saturation2          2
#define Saturation1          3
#define Saturation0          4
#define Saturation_1         5
#define Saturation_2         6
#define Saturation_3         7
#define Saturation_4         8

// Brightness
#define Brightness4          0
#define Brightness3          1
#define Brightness2          2
#define Brightness1          3
#define Brightness0          4
#define Brightness_1         5
#define Brightness_2         6
#define Brightness_3         7
#define Brightness_4         8


// Contrast
#define Contrast4            0
#define Contrast3            1
#define Contrast2            2
#define Contrast1            3
#define Contrast0            4
#define Contrast_1           5
#define Contrast_2           6
#define Contrast_3           7
#define Contrast_4           8


// Special effects
#define Antique                      0
#define Bluish                       1
#define Greenish                     2
#define Reddish                      3
#define BW                           4
#define Negative                     5
#define BWnegative                   6
#define Normal                       7
#define Sepia                        8
#define Overexposure                 9
#define Solarize                     10
#define Blueish                     11
#define Yellowish                    12

struct sensor_reg {
	uint16_t reg;
	uint16_t val;
};

extern const struct sensor_reg OV2640_QVGA[];
extern const struct sensor_reg OV2640_JPEG_INIT[];
extern const struct sensor_reg OV2640_YUV422[];
extern const struct sensor_reg OV2640_JPEG[];
extern const struct sensor_reg OV2640_160x120_JPEG[];
extern const struct sensor_reg OV2640_176x144_JPEG[];
extern const struct sensor_reg OV2640_320x240_JPEG[];
extern const struct sensor_reg OV2640_352x288_JPEG[];
extern const struct sensor_reg OV2640_640x480_JPEG[];
extern const struct sensor_reg OV2640_800x600_JPEG[];
extern const struct sensor_reg OV2640_1024x768_JPEG[];
extern const struct sensor_reg OV2640_1280x1024_JPEG[];
extern const struct sensor_reg OV2640_1600x1200_JPEG[];

#endif
