#include "button.h"

#include "thread.h"
#include "event.h"
#include "manage/can.h"

#define BUTTON_DATA_SEND_PERIOD  100 // ms
#define BUTTON_LONG_PRESS_TIME  3000 // ms

static bool buttonThreadInit(void);
static bool buttonThreadUpdate(void);
static void buttonThreadISR(void *arg);

static uint32_t button_send_time = 0;

static void canSendButtonState(uint32_t button_data);

__attribute__((section(".thread"))) 
static volatile thread_t thread_obj = 
{
  .name = "button",
  .is_enable = true,
  .init = buttonThreadInit,
  .update = buttonThreadUpdate
};

bool buttonThreadInit(void)
{
  swtimer_handle_t timer_ch;
  timer_ch = swtimerGetHandle();
  if (timer_ch >= 0)
  {
    swtimerSet(timer_ch, 10, LOOP_TIME, buttonThreadISR, NULL);
    swtimerStart(timer_ch);
  }
  else
  {
    logPrintf("[NG] buttonThreadInit()\n     swtimerGetHandle()\n");
  }
  return true;
}

bool buttonThreadUpdate(void)
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
          button_data |= (0x02 << ((i%2)*4 + (i/2)*8));  // long press
        }
        else
        {
          button_data |= (0x01 << ((i%2)*4 + (i/2)*8));  // short press
        }
      }
    }

    if (canObj()->isOpen() == true)
    {
      canSendButtonState(button_data);
    }
  }

  return true;
}

void buttonThreadISR(void *arg)
{  

}

static void canSendButtonState(uint32_t button_data)
{
  static uint8_t tx_cnt = 0;
  can_msg_t msg;
  
  msg.frame   = CAN_CLASSIC;
  msg.id_type = CAN_STD;
  msg.dlc     = CAN_DLC_4;
  msg.id      = ID_BUTTON_STATE;
  msg.length  = 4;

  msg.data[0] = tx_cnt++;
  msg.data[1] = (uint8_t)(button_data >> 0);   // 버튼 1,2
  msg.data[2] = (uint8_t)(button_data >> 8);   // 버튼 3,4
  msg.data[3] = (uint8_t)(button_data >> 16);  // 버튼 5,6

  canMsgWrite(_DEF_CAN1, &msg, 10);
  canHandleTxMessage(&msg);
}

