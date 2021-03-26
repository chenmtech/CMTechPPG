/*
 * App_PPGFunc.h : PPG application Function Model source file
 * Written by Chenm
 */

#include "hal_mcu.h"
#include "App_PPGFunc.h"
#include "CMUtil.h"
#include "service_PPG.h"
#include "CMTechPPG.h"
#include "Dev_MAX30102.h"

#define PPG_PACK_BYTE_NUM 19 // byte number per PPG packet, 1+9*2
#define PPG_MAX_PACK_NUM 255 // max packet num

static uint8 taskId; // taskId of application

// the byte number of the current ppg data packet, from 0 to PPG_MAX_PACK_NUM
static uint8 pckNum = 0;

// ppg packet buffer
static uint8 ppgBuff[PPG_PACK_BYTE_NUM] = {0};

// pointer to the current position in ppgBuff
static uint8* pPpgBuff;

// ppg packet structure sent out
static attHandleValueNoti_t ppgNoti;

// the callback function to process the PPG data read from MAX30102
static void processPpgSignal(uint16 data);

// 初始化
extern void PPGFunc_Init(uint8 taskID)
{ 
  taskId = taskID;
  // 判断芯片是否上电
  while(!MAX30102_IsPowerOn());
  // 配置MAX30102
  MAX30102_Setup();
  // 进入低功耗模式
  MAX30102_Shutdown();
}

extern void PPGFunc_SetPpgSampling(bool start)
{
  pckNum = 0;
  pPpgBuff = ppgBuff;
  osal_clear_event(taskId, PPG_PACKET_NOTI_EVT);
  if(start) // 开始采样
  {
    MAX30102_WakeUp(); // 唤醒
    MAX30102_Start(); // 开始
  } 
  else
  {
    MAX30102_Stop(); // 停止
    MAX30102_Shutdown(); // 进入低功耗模式
  }
}

extern void PPGFunc_SendPpgPacket(uint16 connHandle)
{
  PPG_PacketNotify( connHandle, &ppgNoti );
}

#pragma vector = P0INT_VECTOR
__interrupt void PORT0_ISR(void)
{ 
  HAL_ENTER_ISR();  // Hold off interrupts.

  // P0_2中断 
  if(P0IFG & 0x04)
  {
    P0IFG &= ~(1<<2);   // clear P0_2 interrupt status flag
    //if(MAX30102_IsDATARDY()) // MAX30102数据中断 
    //{
      uint16 ppg = 0;
      if(MAX30102_ReadPpgSample(&ppg))
        processPpgSignal(ppg);
    //}
  }
  
  P0IF = 0;           //clear P0 interrupt flag
  
  HAL_EXIT_ISR();   // Re-enable interrupts.  
}

static void processPpgSignal(uint16 data)
{
  if(pPpgBuff == ppgBuff)
  {
    *pPpgBuff++ = pckNum;
    pckNum = (pckNum == PPG_MAX_PACK_NUM) ? 0 : pckNum+1;
  }
  *pPpgBuff++ = LO_UINT16(data);  
  *pPpgBuff++ = HI_UINT16(data);
  
  if(pPpgBuff-ppgBuff >= PPG_PACK_BYTE_NUM)
  {
    osal_memcpy(ppgNoti.value, ppgBuff, PPG_PACK_BYTE_NUM);
    ppgNoti.len = PPG_PACK_BYTE_NUM;
    osal_set_event(taskId, PPG_PACKET_NOTI_EVT);
    pPpgBuff = ppgBuff;
  }
}