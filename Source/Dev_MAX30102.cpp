/*
* MAX30102: ���������ʴ������豸����
* Written by Chenm 2020-12-07
*/


#include "Dev_MAX30102.h"
#include "hal_mcu.h"

/*
* ����
*/
#define I2C_ADDR 0x57   //MAX30102��I2C��ַ

static MAX30102_DataCB_t pfnMAXDataCB; // callback function processing data 

static void initIntPin();
static void readOneSampleData();

/*
* �ֲ�����
*/

// ����INT�ж�
static void initIntPin()
{
  //P0.1 INT�ܽ�����  
  //�ȹ�P0.1��INT�ж�
  P0IEN &= ~(1<<1);
  P0IFG &= ~(1<<1);   // clear P0_1 interrupt status flag
  P0IF = 0;           //clear P0 interrupt flag  
  
  //����P0.1��INT �ж�
  P0SEL &= ~(1<<1); //GPIO
  P0DIR &= ~(1<<1); //Input
  PICTL |= (1<<0);  //�½��ش���
  //////////////////////////
  
  //��P0.1 INT�ж�
  P0IEN |= (1<<1);    // P0_1 interrupt enable
  P0IE = 1;           // P0 interrupt enable  
}

/*
* ��������
*/
extern void MAX30102_Init(MAX30102_DataCB_t pfnCB)
{
  pfnMAXDataCB = pfnCB;
  initIntPin();
}

//����������Slave Address��SCLKƵ��
extern void MAX30102_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

//ֹͣMAX30102
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