#include "ap_hw_test.h"


uint16_t o_x       = 0;
uint8_t  o_y       = 0;
uint8_t  test_mode = 0;
uint32_t pre_time  = 0;

#ifdef _USE_HW_CLI
static void cliHwTest(cli_args_t *args);
#endif
void apHwTestInit(void)
{
  pre_time  = millis();
#ifdef _USE_HW_CLI
  cliAdd("hw_test", cliHwTest);
#endif
}

void apHwTestMain(void)
{
  apHwTestUpdate();
}

void apHwTestUpdate(void)
{
  if (lcdDrawAvailable() == true)
  {
    lcdClearBuffer(black);

    switch (test_mode)
    {
      case 0:
        lcdSetFont(LCD_FONT_07x10);
        lcdPrintf(0, 0, white, "SSD1322 256x64 TEST");
        lcdPrintf(0, 16, white, "FPS : %d", lcdGetFps());
        lcdDrawFillRect(o_x, 32, 32, 16, white);
        o_x += 4;
        if (o_x >= LCD_WIDTH - 32) o_x = 0;
        break;

      case 1:
        for (int y = 0; y < LCD_HEIGHT; y += 8)
        {
          for (int x = 0; x < LCD_WIDTH; x += 8)
          {
            if ((x + y) % 16 == 0)
              lcdDrawFillRect(x, y, 8, 8, white);
          }
        }
        break;

      case 2:
        for (int i = 0; i < LCD_WIDTH + LCD_HEIGHT; i += 8)
        {
          lcdDrawLine(0, i, i, 0, white);
        }
        break;

      case 3:
        lcdSetFont(LCD_FONT_07x10);
        for (int i = 0; i < 4; i++)
        {
          int y_pos = (i * 16 + o_y) % LCD_HEIGHT;
          lcdPrintf(0, y_pos, white, "Scroll Text Line %d", i + 1);
        }
        o_y = (o_y + 1) % LCD_HEIGHT;
        break;
    }

    if (millis() - pre_time >= 3000)
    {
      pre_time  = millis();
      test_mode = (test_mode + 1) % 4;
      o_x       = 0;
      o_y       = 0;
    }

    lcdRequestDraw();
  }
}

#ifdef _USE_HW_CLI
void cliHwTest(cli_args_t *args)
{
  bool ret = false;
  int  i   = 0;

  if (args->argc == 1 && args->isStr(0, "test") == true)
  {

    // sd-card test

    while (cliKeepLoop())
    {
      // oled

      // button test (led bilink, button print)

      // state led

      // can test (rx data print)

      // 485 (tx and input char print)

      // 232 (loop back)

      cliPrintf("Test:%d\n", i++);
      cliMoveUp(1);
      delay(100);
    }
    cliMoveDown(1);

    lcdClearBuffer(black);
    lcdUpdateDraw();

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("lcd test\n");
  }
}
#endif
