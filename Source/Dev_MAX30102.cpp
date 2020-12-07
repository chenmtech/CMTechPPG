/*
* MAX30102: 脉搏和心率传感器设备驱动
* Written by Chenm 2020-12-07
*/


#include "Dev_MAX30102.h"
#include "hal_mcu.h"

/*
* 常量
*/
#define I2C_ADDR 0x57   //MAX30102的I2C地址

static MAX30102_DataCB_t pfnMAXDataCB; // callback function processing data 

static void initIntPin();
static void readOneSampleData();

/*
* 局部函数
*/

// 设置INT中断
static void initIntPin()
{
  //P0.1 INT管脚配置  
  //先关P0.1即INT中断
  P0IEN &= ~(1<<1);
  P0IFG &= ~(1<<1);   // clear P0_1 interrupt status flag
  P0IF = 0;           //clear P0 interrupt flag  
  
  //配置P0.1即INT 中断
  P0SEL &= ~(1<<1); //GPIO
  P0DIR &= ~(1<<1); //Input
  PICTL |= (1<<0);  //下降沿触发
  //////////////////////////
  
  //开P0.1 INT中断
  P0IEN |= (1<<1);    // P0_1 interrupt enable
  P0IE = 1;           // P0 interrupt enable  
}

/*
* 公共函数
*/
extern void MAX30102_Init(MAX30102_DataCB_t pfnCB)
{
  pfnMAXDataCB = pfnCB;
  initIntPin();
}

//启动：设置Slave Address和SCLK频率
extern void MAX30102_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

//停止MAX30102
extern void MAX30102_Stop()
{
  HalI2CDisable();
}

#pragma vector = P0INT_VECTOR
__interrupt void PORT0_ISR(void)
{ 
  HAL_ENTER_ISR();  // Hold off interrupts.
  
  P0IFG &= 0xFD; //~(1<<1);   //clear P0_1 IFG 
  P0IF = 0;      //clear P0 interrupt flag

  readOneSampleData();
  
  HAL_EXIT_ISR();   // Re-enable interrupts.  
}

static void readOneSampleData()
{
  pfnMAXDataCB(100, 120);
}