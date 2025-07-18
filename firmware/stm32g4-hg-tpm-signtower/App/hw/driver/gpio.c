#include "gpio.h"
#include "cli.h"


#ifdef _USE_HW_GPIO

typedef struct
{
  GPIO_TypeDef *port;
  uint32_t      pin;
  uint8_t       mode;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
  bool          init_value;
  const char   *p_name;
} gpio_tbl_t;

const gpio_tbl_t gpio_tbl[HW_GPIO_MAX_CH] =
{
  {P_OUT1_GPIO_Port,    P_OUT1_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT1"   }, // LIGHT_1 - RED
  {P_OUT2_GPIO_Port,    P_OUT2_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT2"   }, // LIGHT_2 - YELLOW
  {P_OUT3_GPIO_Port,    P_OUT3_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT3"   }, // LIGHT_3 - GREEN
  {P_OUT4_GPIO_Port,    P_OUT4_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT4"   }, // LIGHT_4 - X
  {P_OUT5_GPIO_Port,    P_OUT5_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT5"   }, // LIGHT_5 - BUZZER
  {P_OUT6_GPIO_Port,    P_OUT6_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT6"   }, // LIGHT_6 - X
  {P_OUT7_GPIO_Port,    P_OUT7_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT7"   }, // LIGHT_7 - X
  {P_OUT8_GPIO_Port,    P_OUT8_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT8"   }, // BLINK_ENABLE, 
  {ID1_GPIO_Port,       ID1_Pin,       _DEF_INPUT_PULLUP, GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "ID1"      },
};


#ifdef _USE_HW_CLI
static void cliGpio(cli_args_t *args);
#endif


bool gpioInit(void)
{
  bool ret = true;

  for (int i = 0; i < GPIO_MAX_CH; i++)
  {
    gpioPinMode(i, gpio_tbl[i].mode);

    if (gpio_tbl[i].mode & _DEF_OUTPUT)
    {
      gpioPinWrite(i, gpio_tbl[i].init_value);
    }
  }

#ifdef _USE_HW_CLI
  cliAdd("gpio", cliGpio);
#endif

  return ret;
}

bool gpioPinMode(uint8_t ch, uint8_t mode)
{
  bool             ret             = true;
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  if (ch >= HW_GPIO_MAX_CH)
  {
    return false;
  }

  switch (mode)
  {
    case _DEF_INPUT:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      break;

    case _DEF_INPUT_PULLUP:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      break;

    case _DEF_INPUT_PULLDOWN:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLDOWN;
      break;

    case _DEF_OUTPUT:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      break;

    case _DEF_OUTPUT_PULLUP:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      break;

    case _DEF_OUTPUT_PULLDOWN:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLDOWN;
      break;
  }

  GPIO_InitStruct.Pin = gpio_tbl[ch].pin;
  HAL_GPIO_Init(gpio_tbl[ch].port, &GPIO_InitStruct);

  return ret;
}

void gpioPinWrite(uint8_t ch, bool value)
{
  if (ch >= HW_GPIO_MAX_CH)
  {
    return;
  }

  if (value)
  {
    HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].on_state);
  }
  else
  {
    HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].off_state);
  }
}

bool gpioPinRead(uint8_t ch)
{
  bool ret = false;

  if (ch >= HW_GPIO_MAX_CH)
  {
    return false;
  }

  if (HAL_GPIO_ReadPin(gpio_tbl[ch].port, gpio_tbl[ch].pin) == gpio_tbl[ch].on_state)
  {
    ret = true;
  }

  return ret;
}

void gpioPinToggle(uint8_t ch)
{
  if (ch >= HW_GPIO_MAX_CH)
  {
    return;
  }

  HAL_GPIO_TogglePin(gpio_tbl[ch].port, gpio_tbl[ch].pin);
}


#ifdef _USE_HW_CLI
void cliGpio(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    for (int i = 0; i < HW_GPIO_MAX_CH; i++)
    {
      cliPrintf("%02d %-16s - %d\n", i, gpio_tbl[i].p_name, gpioPinRead(i));
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    while (cliKeepLoop())
    {
      for (int i = 0; i < HW_GPIO_MAX_CH; i++)
      {
        cliPrintf("%d", gpioPinRead(i));
      }
      cliPrintf("\n");
      delay(100);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    while (cliKeepLoop())
    {
      cliPrintf("gpio read %d : %d\n", ch, gpioPinRead(ch));
      delay(100);
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "write") == true)
  {
    uint8_t ch;
    uint8_t data;

    ch   = (uint8_t)args->getData(1);
    data = (uint8_t)args->getData(2);

    gpioPinWrite(ch, data);

    cliPrintf("gpio write %d : %d\n", ch, data);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("gpio info\n");
    cliPrintf("gpio show\n");
    cliPrintf("gpio read ch[0~%d]\n", HW_GPIO_MAX_CH - 1);
    cliPrintf("gpio write ch[0~%d] 0:1\n", HW_GPIO_MAX_CH - 1);
  }
}
#endif


#endif
