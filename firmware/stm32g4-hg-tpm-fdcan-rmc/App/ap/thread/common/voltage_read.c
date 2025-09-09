/*
 * voltage_read.c
 *
 *  Created on: Feb 5, 2025
 *      Author: user
 */
#include "voltage_read.h"

#include "thread.h"
#include "event.h"
#include "manage/can.h"

#define ADC_REF_VOLTAGE   (float)3.3f //  3.3V
#define ADC_RESOLUTION    (float)4096.0f  // 12bit
#define ADC_REG_R1        (float)470.0f // Kohm
#define ADC_REG_R2        (float)33.0f  // Kohm
#define ADC_DATA_10X      (float)10.0f

static bool voltageReadThreadinit(void);
static bool voltageReadThreadupdate(void);
static void canSendvoltageReadState(uint16_t* voltage_data);

float voltage_read_ret[HW_ADC_MAX_CH] = {0.0f,};
uint16_t voltage_data[HW_ADC_MAX_CH] = {0,};

enum
{
  VIN1 = 0,
  VIN2,
  VIN3
};

__attribute__((section(".thread")))
static volatile thread_t thread_obj =
{
  .name = "voltageRead",
  .is_enable = true,
  .init = voltageReadThreadinit,
  .update = voltageReadThreadupdate
};

bool voltageReadThreadinit(void)
{
  return true;
}

bool voltageReadThreadupdate(void)
{
  static uint32_t pre_time;

  if (millis()-pre_time >= 100)
  {
    pre_time = millis();

    for(int i = 0 ; i < HW_ADC_MAX_CH ; i++)
    {
      //voltage_data[i] = (uint16_t)( ( (float)adcRead(i)*( ADC_REF_VOLTAGE / ADC_RESOLUTION ) ) * ( ( ADC_REG_R1 + ADC_REG_R2 ) / ADC_REG_R2 ) * ADC_DATA_10X);
      voltage_read_ret[i] = (float)adcRead(i)*( ADC_REF_VOLTAGE / ADC_RESOLUTION ); //  ADC값을 3.3Vref 기준으로 변경
      voltage_read_ret[i] = voltage_read_ret[i] * ( ( ADC_REG_R1 + ADC_REG_R2 ) / ADC_REG_R2 ) ; // ADC값을 역분압 법칙으로 실제 전압으로 변환
      voltage_data[i] = (uint16_t)( voltage_read_ret[i] * ADC_DATA_10X ); // 소수점 1번째 자리까지 표현 하기 위해 10배 후 정수값으로 변환
    }

    if (canObj()->isOpen() == true)
    {
      canSendvoltageReadState(voltage_data);
    }
  }
  return true;
}

static void canSendvoltageReadState(uint16_t* voltage_data)
{
  static uint8_t tx_cnt = 0;
  can_msg_t msg;

  msg.frame   = CAN_CLASSIC;
  msg.id_type = CAN_STD;
  msg.dlc     = CAN_DLC_7;
  msg.id      = ID_VOLTAGE_READ;
  msg.length  = 7; // count(1) + volt_data(2) * 3

  msg.data[0] = tx_cnt++;

  msg.data[1] = voltage_data[VIN1] & 0xFF;
  msg.data[2] = ( voltage_data[VIN1]  >> 8 ) & 0xFF;
  msg.data[3] = voltage_data[VIN2] & 0xFF;
  msg.data[4] = ( voltage_data[VIN2]  >> 8 ) & 0xFF;
  msg.data[5] = voltage_data[VIN3] & 0xFF;
  msg.data[6] = ( voltage_data[VIN3]  >> 8 ) & 0xFF;

  canMsgWrite(_DEF_CAN1, &msg, 10);
  canHandleTxMessage(&msg);
}
