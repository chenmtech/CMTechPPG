/*
 * MAX30102: 脉搏和心率传感器MAX30102驱动
 * written by chenm 2020-12-07
*/

#ifndef DEV_MAX30102_H
#define DEV_MAX30102_H

typedef void (*MAX30102_DataCB_t)(uint16 red, uint16 ir); // callback function to handle one sample data

extern void MAX30102_Init(MAX30102_DataCB_t pfnCB);

extern void MAX30102_Start();

extern void MAX30102_Stop();

 
#endif

