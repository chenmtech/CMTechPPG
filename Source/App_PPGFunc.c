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

// the number of the current ppg data packet, from 0 to PPG_MAX_PACK_NUM
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
  
  // 配置MAX30102
  MAX30102_Setup();
  delayus(1000);
  MAX30102_Stop();
  delayus(1000);
  MAX30102_Shutdown();
  delayus(1000);
}

extern void PPGFunc_SetPpgSampling(bool start)
{
  pckNum = 0;
  pPpgBuff = ppgBuff;
  osal_clear_event(taskId, PPG_PACKET_NOTI_EVT);
  if(start)
  {
    MAX30102_WakeUp();
    delayus(1000);
    MAX30102_Start();
  } 
  else
  {
    MAX30102_Stop();
    delayus(1000);
    MAX30102_Shutdown();
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

  // P0_2中断, 即MAX30102数据中断  
  if(P0IFG & 0x04)
  {
    uint16 ppg = 0;
    MAX30102_ReadPpgSample(&ppg);
    P0IFG &= ~(1<<2);   // clear P0_2 interrupt status flag
    
    processPpgSignal(ppg);
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