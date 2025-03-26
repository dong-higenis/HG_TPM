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
  {CAN_STB_GPIO_Port,   CAN_STB_Pin,   _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "CAN_STB"  },
  {P_OUT1_GPIO_Port,    P_OUT1_Pin,    _DEF_OUTPUT,       GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_LOW,  "P_OUT1"   }, // sensor 24V
  {P_IN1_GPIO_Port,     P_IN1_Pin,     _DEF_INPUT,        GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "P_IN1"    }, // sensor result 1
  {P_IN2_GPIO_Port,     P_IN2_Pin,     _DEF_INPUT,        GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "P_IN2"    }, // sensor result 2 
  {P_IN3_GPIO_Port,     P_IN3_Pin,     _DEF_INPUT,        GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "P_IN3"    }, // sensor result 3
  {P_IN4_GPIO_Port,     P_IN4_Pin,     _DEF_INPUT,        GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "P_IN4"    }, // sensor result 4
  {ID0_GPIO_Port,       ID0_Pin,       _DEF_INPUT_PULLUP, GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "ID0"      },
  {ID1_GPIO_Port,       ID1_Pin,       _DEF_INPUT_PULLUP, GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "ID1"      },
  {ID2_GPIO_Port,       ID2_Pin,       _DEF_INPUT_PULLUP, GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH, "ID2"      },
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
