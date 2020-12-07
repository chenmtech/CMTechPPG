/*
 * App_PPGFunc.h : PPG application Function Model source file
 * Written by Chenm
 */

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
// pointer to the ppg buff
static uint8* pPpgBuff;
// ppg packet structure sent out
static attHandleValueNoti_t ppgNoti;

static void processPpgSignal(uint16 red, uint16 ir);

extern void PPGFunc_Init(uint8 taskID)
{ 
  taskId = taskID;
  
  // initilize the MAX30102 and set the data process callback function
  MAX30102_Init(processPpgSignal);
  
  delayus(1000);
}

extern void PPGFunc_SetPpgSampling(bool start)
{
  pckNum = 0;
  pPpgBuff = ppgBuff;
  osal_clear_event(taskId, PPG_PACKET_NOTI_EVT);
  if(start)
  {
    ADS1x9x_WakeUp(); 
    // 这里一定要延时，否则容易死机
    delayus(1000);
    ADS1x9x_StartConvert();
    delayus(1000);
  } 
  else
  {
    ADS1x9x_StopConvert();
    ADS1x9x_StandBy();
    delayus(2000);
  }
}

extern void PPGFunc_SendPpgPacket(uint16 connHandle)
{
  PPG_PacketNotify( connHandle, &ppgNoti );
}

static void processPpgSignal(uint16 red, uint16 ir)
{
  if(pPpgBuff == ppgBuff)
  {
    *pPpgBuff++ = pckNum;
    pckNum = (pckNum == PPG_MAX_PACK_NUM) ? 0 : pckNum+1;
  }
  *pPpgBuff++ = LO_UINT16(red);  
  *pPpgBuff++ = HI_UINT16(red);
  
  if(pPpgBuff-ppgBuff >= PPG_PACK_BYTE_NUM)
  {
    osal_memcpy(ppgNoti.value, ppgBuff, PPG_PACK_BYTE_NUM);
    ppgNoti.len = PPG_PACK_BYTE_NUM;
    osal_set_event(taskId, PPG_PACKET_NOTI_EVT);
    pPpgBuff = ppgBuff;
  }
}