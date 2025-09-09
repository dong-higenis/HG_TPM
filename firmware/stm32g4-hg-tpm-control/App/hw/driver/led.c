#include "led.h"
#include "cli.h"


#ifdef _USE_HW_LED


const typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
  const char   *p_name;
} led_tbl_t;

static led_tbl_t led_tbl[LED_MAX_CH] =
{
  { STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET, GPIO_PIN_SET, "STATUS_LED" },
  { LINK_LED_GPIO_Port,   LINK_LED_Pin,   GPIO_PIN_RESET, GPIO_PIN_SET, "LINK_LED"   },
};


#ifdef _USE_HW_LED
static void cliLed(cli_args_t *args);
#endif


bool ledInit(void)
{
  ledOn(_DEF_LED1);

#ifdef _USE_HW_CLI
  cliAdd("led", cliLed);
#endif

  return true;
}

bool ledIsOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return false;

  return HAL_GPIO_ReadPin(led_tbl[ch].port, led_tbl[ch].pin) == led_tbl[ch].on_state;
}

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].on_state);
}

void ledOff(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].off_state);
}

void ledToggle(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  HAL_GPIO_TogglePin(led_tbl[ch].port, led_tbl[ch].pin);
}

#ifdef _USE_HW_CLI
void cliLed(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    for (int i = 0; i < LED_MAX_CH; i++)
    {
      cliPrintf("%02d %-16s - %d\n", i, led_tbl[i].p_name, ledIsOn(i));
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    while (cliKeepLoop())
    {
      for (int i = 0; i < LED_MAX_CH; i++)
      {
        cliPrintf("%d", ledIsOn(i));
      }
      cliPrintf("\n");
      delay(100);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "toggle") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    ledToggle(ch);
    cliPrintf("led toggle [%s]\n", led_tbl[ch].p_name);
    ret = true;   
  }

  if (args->argc == 2 && args->isStr(0, "on") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    ledOn(ch);
    cliPrintf("led on [%s]\n", led_tbl[ch].p_name);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "off") == true)
  {
    uint8_t ch;    
    ch = (uint8_t)args->getData(1);

    ledOff(ch);

    cliPrintf("led off [%s]\n", led_tbl[ch].p_name);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("led info\n");
    cliPrintf("led show\n");
    cliPrintf("led toggle ch[0~%d]\n", LED_MAX_CH - 1);
    cliPrintf("led on ch[0~%d]\n", LED_MAX_CH - 1);
    cliPrintf("led off ch[0~%d]\n", LED_MAX_CH - 1);
  }
}
#endif

#endif
