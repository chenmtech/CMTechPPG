/*
 * MAX30102: 脉搏和心率传感器设备驱动
 * written by chenm
*/

#ifndef DEV_MAX30102_H
#define DEV_MAX30102_H

#include "hal_i2c.h"

typedef void (*MAX30102_DataCB_t)(uint16 red, uint16 ir); // callback function to handle one sample data

extern void MAX30102_Init(MAX30102_DataCB_t pfnCB);

extern void MAX30102_Start();

extern void MAX30102_Stop();

 
#endif

