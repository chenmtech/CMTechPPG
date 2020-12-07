/*
* MAX30102: ���������ʴ������豸����
* Written by Chenm 2020-12-07
*/


#include "Dev_MAX30102.h"
#include "hal_mcu.h"
#include "CMUtil.h"

/*
* ����
*/
#define I2C_ADDR 0x57   //MAX30102��I2C��ַ

// Status Registers
static const uint8 MAX30102_INTSTAT1 =		0x00;
static const uint8 MAX30102_INTSTAT2 =		0x01;
static const uint8 MAX30102_INTENABLE1 =		0x02;
static const uint8 MAX30102_INTENABLE2 =		0x03;

// FIFO Registers
static const uint8 MAX30102_FIFOWRITEPTR = 	0x04;
static const uint8 MAX30102_FIFOOVERFLOW = 	0x05;
static const uint8 MAX30102_FIFOREADPTR = 	0x06;
static const uint8 MAX30102_FIFODATA =		0x07;

// Configuration Registers
static const uint8 MAX30102_FIFOCONFIG = 		0x08;
static const uint8 MAX30102_MODECONFIG = 		0x09;
static const uint8 MAX30102_PARTICLECONFIG = 	0x0A;    // Note, sometimes listed as "SPO2" config in datasheet (pg. 11)
static const uint8 MAX30102_LED1_PULSEAMP = 	0x0C;
static const uint8 MAX30102_LED2_PULSEAMP = 	0x0D;
static const uint8 MAX30102_LED3_PULSEAMP = 	0x0E;
static const uint8 MAX30102_LED_PROX_AMP = 	0x10;
static const uint8 MAX30102_MULTILEDCONFIG1 = 0x11;
static const uint8 MAX30102_MULTILEDCONFIG2 = 0x12;

// Die Temperature Registers
static const uint8 MAX30102_DIETEMPINT = 		0x1F;
static const uint8 MAX30102_DIETEMPFRAC = 	0x20;
static const uint8 MAX30102_DIETEMPCONFIG = 	0x21;

// Proximity Function Registers
static const uint8 MAX30102_PROXINTTHRESH = 	0x30;

// Part ID Registers
static const uint8 MAX30102_REVISIONID = 		0xFE;
static const uint8 MAX30102_PARTID = 			0xFF;    // Should always be 0x15. Identical to MAX30102.

// MAX30102 Commands
// Interrupt configuration (pg 13, 14)
static const uint8 MAX30102_INT_A_FULL_MASK =		(uint8)~0x80;
static const uint8 MAX30102_INT_A_FULL_ENABLE = 	0x80;
static const uint8 MAX30102_INT_A_FULL_DISABLE = 	0x00;

static const uint8 MAX30102_INT_DATA_RDY_MASK = (uint8)~0x40;
static const uint8 MAX30102_INT_DATA_RDY_ENABLE =	0x40;
static const uint8 MAX30102_INT_DATA_RDY_DISABLE = 0x00;

static const uint8 MAX30102_INT_ALC_OVF_MASK = (uint8)~0x20;
static const uint8 MAX30102_INT_ALC_OVF_ENABLE = 	0x20;
static const uint8 MAX30102_INT_ALC_OVF_DISABLE = 0x00;

static const uint8 MAX30102_INT_PROX_INT_MASK = (uint8)~0x10;
static const uint8 MAX30102_INT_PROX_INT_ENABLE = 0x10;
static const uint8 MAX30102_INT_PROX_INT_DISABLE = 0x00;

static const uint8 MAX30102_INT_DIE_TEMP_RDY_MASK = (uint8)~0x02;
static const uint8 MAX30102_INT_DIE_TEMP_RDY_ENABLE = 0x02;
static const uint8 MAX30102_INT_DIE_TEMP_RDY_DISABLE = 0x00;

static const uint8 MAX30102_SAMPLEAVG_MASK =	(uint8)~0xE0;
static const uint8 MAX30102_SAMPLEAVG_1 = 	0x00;
static const uint8 MAX30102_SAMPLEAVG_2 = 	0x20;
static const uint8 MAX30102_SAMPLEAVG_4 = 	0x40;
static const uint8 MAX30102_SAMPLEAVG_8 = 	0x60;
static const uint8 MAX30102_SAMPLEAVG_16 = 	0x80;
static const uint8 MAX30102_SAMPLEAVG_32 = 	0xA0;

static const uint8 MAX30102_ROLLOVER_MASK = 	0xEF;
static const uint8 MAX30102_ROLLOVER_ENABLE = 0x10;
static const uint8 MAX30102_ROLLOVER_DISABLE = 0x00;

static const uint8 MAX30102_A_FULL_MASK = 	0xF0;

// Mode configuration commands (page 19)
static const uint8 MAX30102_SHUTDOWN_MASK = 	0x7F;
static const uint8 MAX30102_SHUTDOWN = 		0x80;
static const uint8 MAX30102_WAKEUP = 			0x00;

static const uint8 MAX30102_RESET_MASK = 		0xBF;
static const uint8 MAX30102_RESET = 			0x40;

static const uint8 MAX30102_MODE_MASK = 		0xF8;
static const uint8 MAX30102_MODE_REDONLY = 	0x02;
static const uint8 MAX30102_MODE_REDIRONLY = 	0x03;
static const uint8 MAX30102_MODE_MULTILED = 	0x07;

// Particle sensing configuration commands (pgs 19-20)
static const uint8 MAX30102_ADCRANGE_MASK = 	0x9F;
static const uint8 MAX30102_ADCRANGE_2048 = 	0x00;
static const uint8 MAX30102_ADCRANGE_4096 = 	0x20;
static const uint8 MAX30102_ADCRANGE_8192 = 	0x40;
static const uint8 MAX30102_ADCRANGE_16384 = 	0x60;

static const uint8 MAX30102_SAMPLERATE_MASK = 0xE3;
static const uint8 MAX30102_SAMPLERATE_50 = 	0x00;
static const uint8 MAX30102_SAMPLERATE_100 = 	0x04;
static const uint8 MAX30102_SAMPLERATE_200 = 	0x08;
static const uint8 MAX30102_SAMPLERATE_400 = 	0x0C;
static const uint8 MAX30102_SAMPLERATE_800 = 	0x10;
static const uint8 MAX30102_SAMPLERATE_1000 = 0x14;
static const uint8 MAX30102_SAMPLERATE_1600 = 0x18;
static const uint8 MAX30102_SAMPLERATE_3200 = 0x1C;

static const uint8 MAX30102_PULSEWIDTH_MASK = 0xFC;
static const uint8 MAX30102_PULSEWIDTH_69 = 	0x00;
static const uint8 MAX30102_PULSEWIDTH_118 = 	0x01;
static const uint8 MAX30102_PULSEWIDTH_215 = 	0x02;
static const uint8 MAX30102_PULSEWIDTH_411 = 	0x03;

//Multi-LED Mode configuration (pg 22)
static const uint8 MAX30102_SLOT1_MASK = 		0xF8;
static const uint8 MAX30102_SLOT2_MASK = 		0x8F;
static const uint8 MAX30102_SLOT3_MASK = 		0xF8;
static const uint8 MAX30102_SLOT4_MASK = 		0x8F;

static const uint8 SLOT_NONE = 				0x00;
static const uint8 SLOT_RED_LED = 			0x01;
static const uint8 SLOT_IR_LED = 				0x02;
static const uint8 SLOT_GREEN_LED = 			0x03;
static const uint8 SLOT_NONE_PILOT = 			0x04;
static const uint8 SLOT_RED_PILOT =			0x05;
static const uint8 SLOT_IR_PILOT = 			0x06;
static const uint8 SLOT_GREEN_PILOT = 		0x07;

static const uint8 MAX_30105_EXPECTEDPARTID = 0x15;



static MAX30102_DataCB_t pfnMAXDataCB; // callback function processing data 

static void initIntPin();
static void writeOneByte(uint8 reg, uint8 data);
static uint8 readOneByte(uint8 reg);
static void readMultipleBytes(uint8 reg, uint8 len, uint8* pBuff);

static void readOneSampleData();

static void bitMask(uint8 reg, uint8 mask, uint8 thing);

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
  IIC_Enable(I2C_ADDR, i2cClock_267KHZ);
}

//ֹͣMAX30102
extern void MAX30102_Stop()
{
  IIC_Disable();
}


//Begin Interrupt configuration
static uint8 getINT1(void) {
  return (readOneByte(MAX30102_INTSTAT1));
}
static uint8 getINT2(void) {
  return (readOneByte(MAX30102_INTSTAT2));
}

static void enableAFULL(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_A_FULL_MASK, MAX30102_INT_A_FULL_ENABLE);
}
static void disableAFULL(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_A_FULL_MASK, MAX30102_INT_A_FULL_DISABLE);
}

static void enableDATARDY(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_DATA_RDY_MASK, MAX30102_INT_DATA_RDY_ENABLE);
}
static void disableDATARDY(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_DATA_RDY_MASK, MAX30102_INT_DATA_RDY_DISABLE);
}

static void enableALCOVF(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_ALC_OVF_MASK, MAX30102_INT_ALC_OVF_ENABLE);
}
static void disableALCOVF(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_ALC_OVF_MASK, MAX30102_INT_ALC_OVF_DISABLE);
}

static void enablePROXINT(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_PROX_INT_MASK, MAX30102_INT_PROX_INT_ENABLE);
}
static void disablePROXINT(void) {
  bitMask(MAX30102_INTENABLE1, MAX30102_INT_PROX_INT_MASK, MAX30102_INT_PROX_INT_DISABLE);
}

static void enableDIETEMPRDY(void) {
  bitMask(MAX30102_INTENABLE2, MAX30102_INT_DIE_TEMP_RDY_MASK, MAX30102_INT_DIE_TEMP_RDY_ENABLE);
}
static void disableDIETEMPRDY(void) {
  bitMask(MAX30102_INTENABLE2, MAX30102_INT_DIE_TEMP_RDY_MASK, MAX30102_INT_DIE_TEMP_RDY_DISABLE);
}

//End Interrupt configuration

static void softReset(void) {
  bitMask(MAX30102_MODECONFIG, MAX30102_RESET_MASK, MAX30102_RESET);

  while (true)
  {
    uint8 response = readOneByte(MAX30102_MODECONFIG);
    if ((response & MAX30102_RESET) == 0) break; //We're done!
    delayus(1000); //Let's not over burden the I2C bus
  }
}

static void shutDown(void) {
  // Put IC into low power mode (datasheet pg. 19)
  // During shutdown the IC will continue to respond to I2C commands but will
  // not update with or take new readings (such as temperature)
  bitMask(MAX30102_MODECONFIG, MAX30102_SHUTDOWN_MASK, MAX30102_SHUTDOWN);
}

static void wakeUp(void) {
  // Pull IC out of low power mode (datasheet pg. 19)
  bitMask(MAX30102_MODECONFIG, MAX30102_SHUTDOWN_MASK, MAX30102_WAKEUP);
}

static void setLEDMode(uint8 mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  bitMask(MAX30102_MODECONFIG, MAX30102_MODE_MASK, mode);
}

static void setADCRange(uint8 adcRange) {
  // adcRange: one of MAX30102_ADCRANGE_2048, _4096, _8192, _16384
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_ADCRANGE_MASK, adcRange);
}

static void setSampleRate(uint8 sampleRate) {
  // sampleRate: one of MAX30102_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_SAMPLERATE_MASK, sampleRate);
}

static void setPulseWidth(uint8 pulseWidth) {
  // pulseWidth: one of MAX30102_PULSEWIDTH_69, _188, _215, _411
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_PULSEWIDTH_MASK, pulseWidth);
}

// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
static void setPulseAmplitudeRed(uint8 amplitude) {
  writeOneByte(MAX30102_LED1_PULSEAMP, amplitude);
}

static void setPulseAmplitudeIR(uint8 amplitude) {
  writeOneByte(MAX30102_LED2_PULSEAMP, amplitude);
}

//
// FIFO Configuration
//

//Set sample average (Table 3, Page 18)
static void setFIFOAverage(uint8 numberOfSamples) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_SAMPLEAVG_MASK, numberOfSamples);
}

//Resets all points to start in a known state
//Page 15 recommends clearing FIFO before beginning a read
static void clearFIFO(void) {
  writeOneByte(MAX30102_FIFOWRITEPTR, 0);
  writeOneByte(MAX30102_FIFOOVERFLOW, 0);
  writeOneByte(MAX30102_FIFOREADPTR, 0);
}

//Enable roll over if FIFO over flows
static void enableFIFORollover(void) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_ROLLOVER_MASK, MAX30102_ROLLOVER_ENABLE);
}

//Disable roll over if FIFO over flows
static void disableFIFORollover(void) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_ROLLOVER_MASK, MAX30102_ROLLOVER_DISABLE);
}

//Set number of samples to trigger the almost full interrupt (Page 18)
//Power on default is 32 samples
//Note it is reverse: 0x00 is 32 samples, 0x0F is 17 samples
static void setFIFOAlmostFull(uint8 numberOfSamples) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_A_FULL_MASK, numberOfSamples);
}

//Read the FIFO Write Pointer
static uint8 getWritePointer() 
{
  return readOneByte(MAX30102_FIFOWRITEPTR);
}

//Read the FIFO Read Pointer
static uint8 getReadPointer() 
{
  return readOneByte(MAX30102_FIFOREADPTR);
}

//Given a register, read it, mask it, and then set the thing
static void bitMask(uint8 reg, uint8 mask, uint8 thing)
{
  // Grab current register context
  uint8 originalContents = readOneByte(reg);

  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  writeOneByte(reg, originalContents | thing);
}

static void writeOneByte(uint8 reg, uint8 data)
{
  uint8 buff[2] = {reg, data};
  IIC_Write(2, buff);
}

static uint8 readOneByte(uint8 reg)
{
  IIC_Write(1, &reg);
  uint8 data = 0;
  IIC_Read(1, &data);
  return data;
}

static void readMultipleBytes(uint8 rAddress, uint8 len, uint8* pBuff)
{
  IIC_Write(1, &rAddress);
  IIC_Read(len, pBuff);
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