#include "tca9548.h"

#ifdef _USE_HW_TCA9548
#include "cli.h"
#include "i2c.h"

#define TCA9548_ADDR 0x70

static bool tca9548FindChip(void);

static bool    is_init  = false;
static bool    is_found = false;
static uint8_t i2c_ch   = _DEF_I2C1;
static uint8_t i2c_addr = TCA9548_ADDR;


#ifdef _USE_HW_CLI
static void cliTca9548(cli_args_t *args);
#endif

bool tca9548Init(void)
{
  bool ret = true; 

  logPrintf("[  ] tca9548Init()\n");

  if (i2cIsBegin(i2c_ch) == false)
  {
    ret = i2cBegin(i2c_ch, 400);
    if (ret == false)
      logPrintf("     i2cBegin() Fail\n");
  }

  if (ret)
  {
    ret = tca9548FindChip();    
    logPrintf("     [%s] Found TCA9548\n", ret ? "OK" : "NG");

    is_found = ret;
  }

  is_init = ret;

  logPrintf("[%s] tca9548Init()\n", is_init ? "OK" : "NG");

#ifdef _USE_HW_CLI
  cliAdd("tca9548", cliTca9548);
#endif
  return ret;
}

bool tca9548IsInit(void)
{
  return is_init;
}

bool tca9548FindChip(void)
{
  bool    ret;
  uint8_t data;

  ret = i2cReadData(i2c_ch, i2c_addr, &data, 1, 50);

  return ret;
}

bool tca9548SelectChannel(uint8_t ch)
{
  if (ch >= TCA9548_MAX_CH) return false;

  uint8_t data = 1 << ch;

  return i2cWriteData(i2c_ch, i2c_addr, &data, 1, 100);
}

#ifdef _USE_HW_CLI
void cliTca9548(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("tca9548 is_init : %d\n", is_init); 
    cliPrintf("tca9548 is_found : %d\n", is_found); 

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("tca9548 info\n");
  }
}
#endif

#endif

