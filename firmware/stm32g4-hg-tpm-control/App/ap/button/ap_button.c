#include "ap_button.h"
#include "can.h"

#define BUTTON_DATA_SEND_PERIOD 100 // ms
#define BUTTON_LONG_PRESS_TIME 3000 // ms


static uint32_t button_send_time = 0;
uint8_t button_state;

void apButtonUpdate(void);
void apSendButtonState(uint32_t button_data);

void apButtonInit(void)
{

}

void apButtonMain(void)
{
  apButtonUpdate();
}

void apButtonUpdate(void)
{
  uint32_t button_data = 0;

  if (millis() - button_send_time >= BUTTON_DATA_SEND_PERIOD)
  {
    button_send_time = millis();

    // 모든 버튼 처리 (2개씩 한 바이트)
    for (int i = 0; i < BUTTON_MAX_CH; i++)
    {
      if (buttonGetPressed(i))
      {
        if (buttonGetPressedTime(i) >= BUTTON_LONG_PRESS_TIME)
        {
          button_data |= (0x02 << ((i % 2) * 4 + (i / 2) * 8)); // long press
        }
        else
        {
          button_data |= (0x01 << ((i % 2) * 4 + (i / 2) * 8)); // short press
        }
      }
    }

    apSendButtonState(button_data);
  }
}

void apSendButtonState(uint32_t button_data)
{
  static uint8_t tx_cnt = 0;
  can_msg_t msg;
  
  msg.frame   = CAN_CLASSIC;
  msg.id_type = CAN_STD;
  msg.dlc     = CAN_DLC_3;
  msg.id      = ID_LCD_BTN_STATE;
  msg.length  = 3;

  msg.data[0] = tx_cnt++;
  msg.data[1] = (uint8_t)(button_data >> 0);   // 버튼 1,2
  msg.data[2] = (uint8_t)(button_data >> 8);   // 버튼 3,4

  canMsgWrite(_DEF_CAN1, &msg, 10);
}
