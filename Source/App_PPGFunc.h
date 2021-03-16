/*
 * App_PPGFunc.h : PPG application Function Model header file
 * Written by Chenm
 */

#ifndef APP_PPGFUNC_H
#define APP_PPGFUNC_H

#include "hal_types.h"

extern void PPGFunc_Init(uint8 taskID); //init
extern void PPGFunc_SetPpgSampling(bool start); // start or stop the PPG sampling
extern void PPGFunc_SendPpgPacket(uint16 connHandle); // send PPG data packet with connHandle

#endif