#include "led.h"

#include "thread.h"
#include "event.h"
#include "manage/can.h"


static bool ledThreadinit(void);
static bool ledThreadupdate(void);
static void ledThreadISR(void *arg);

__attribute__((section(".thread"))) 
static volatile thread_t thread_obj = 
{
  .name = "led",
  .is_enable = true,
  .init = ledThreadinit,
  .update = ledThreadupdate
};

bool ledThreadinit(void)
{
  swtimer_handle_t timer_ch;
  timer_ch = swtimerGetHandle();
  if (timer_ch >= 0)
  {
    swtimerSet(timer_ch, 10, LOOP_TIME, ledThreadISR, NULL);
    swtimerStart(timer_ch);
  }
  else
  {
    logPrintf("[NG] ledThreadinit()\n     swtimerGetHandle()\n");
  }    
  return true;
}

bool ledThreadupdate(void)
{
  static uint32_t pre_time;

  if (millis()-pre_time >= 500)
  {
    pre_time = millis();
    ledToggle(_DEF_LED1);
  }    
  return true;
}

void ledThreadISR(void *arg)
{
  if (canObj()->isOpen() == true)
  {
    ledOn(_DEF_LED2);
  }
  else
  {
    ledOff(_DEF_LED2);
  }
}
