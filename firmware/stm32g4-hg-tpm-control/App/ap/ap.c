#include "ap.h"
#include "button/ap_button.h"
#include "hwTest/ap_hw_test.h"

void apInit(void)
{
  cliOpen(HW_UART_CH_DEBUG, 115200);
  logBoot(true);

  //  test  read Txt
  test_fatfs();

  apHwTestInit();
}

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();
  while (1)
  {
    if (millis() - pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }
    cliMain();
    canMain();
    apButtonMain();
    apHwTestMain();
  }
}
