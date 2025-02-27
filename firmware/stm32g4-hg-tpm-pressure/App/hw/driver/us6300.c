#include "us6300.h"


#ifdef _USE_HW_US6300
#include "cli.h"
#include "i2c.h"
#include "tca9548.h"

#define US6300_ADDR            0x4C
#define US6300_RES             0.5
#define US6300_OFFSET          1000
#define US6300_MBAR_MIN        -500
#define US6300_MBAR_MAX        500

// https://www.sensorsone.com/psi-to-mbar-conversion-table/
#define CONV_MBAR_TO_PSI(mBar) mBar / 68.9476 


#ifdef _USE_HW_CLI
static void cliUs6300(cli_args_t *args);
#endif

static bool us6300FindChip(void);
static bool us6300FindChipAll(void);

static bool    is_init  = false;
static bool    is_found = false;
static uint8_t i2c_ch   = _DEF_I2C1;
static uint8_t i2c_addr = US6300_ADDR;

bool us6300Init(void)
{
  bool ret = true;

  logPrintf("[  ] us6300Init()\n");


  if (i2cIsBegin(i2c_ch) == false)
  {
    ret = i2cBegin(i2c_ch, 400);

    if (ret == false)
    {
      logPrintf("     i2cBegin() Fail\n");
    }
  }

  if (ret)
  {
    ret = us6300FindChipAll();

    is_found = ret;
  }

  is_init = ret;

  logPrintf("[%s] us6300Init()\n", is_init ? "OK" : "NG");

#ifdef _USE_HW_CLI
  cliAdd("us6300", cliUs6300);
#endif
  return ret;
}

bool us6300IsInit(void)
{
  return is_init;
}

bool us6300FindChip(void)
{
  bool    ret;
  uint8_t data;

  ret = i2cReadData(i2c_ch, i2c_addr, &data, 1, 50);

  return ret;
}

bool us6300FindChipAll(void)
{
  bool ret = false;

  logPrintf("[  ] us6300FindChipAll()\n");

  for (uint8_t ch = 0; ch < TCA9548_MAX_CH; ch++)
  {
    if (tca9548SelectChannel(ch) == false)
    {
      continue;
    }

    ret = us6300FindChip();
    logPrintf("     [%s] Found US6300 ch.%d\n", ret ? "OK" : "NG", ch);
    
    ret = true;    
  }

  return ret;
}

bool us6300GetInfo(us6300_info_t *p_info)
{
  bool    ret;
  uint8_t i2c_data[2];

  ret = i2cReadData(i2c_ch, i2c_addr, i2c_data, 2, 30);
  if (ret)
  {
    p_info->adc  = (i2c_data[0] << 8) | (i2c_data[1] << 0);
    p_info->mBar = ((int32_t)p_info->adc - (int32_t)US6300_OFFSET) * US6300_RES;
    p_info->psi  = CONV_MBAR_TO_PSI(p_info->mBar);

    p_info->mBar_x100 = (int32_t)(p_info->mBar * 100.0f);
    p_info->psi_x100  = (int32_t)(p_info->psi * 100.0f);

    p_info->mmHg = p_info->mBar * 750.062 / 1000.f; // 1Bar == 750.062 mmHg
  }
  else
  {
    p_info->adc      = 0;
    p_info->mBar     = 0;
    p_info->psi      = 0.0;
    p_info->psi_x100 = 0;
    p_info->mmHg     = 0.0;
  }

  return ret;
}

bool us6300GetInfoChannel(uint8_t ch, us6300_info_t *p_info)
{
  bool ret = false;

  ret = tca9548SelectChannel(ch);
  ret = us6300GetInfo(p_info);

  return ret;
}

// TODO : [23.10.13] adc -> mBar , mBar 값 -> psi로 변환. (psi 값은 저장)
#ifdef _USE_HW_CLI
void cliUs6300(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 2 && args->isStr(0, "info") == true)
  {
    us6300_info_t info;
    uint8_t ch = (uint8_t)args->getData(1);

    cliPrintf("is_init : %d\n", is_init);

    us6300GetInfoChannel(ch, &info);

    cliPrintf("adc     : %d (0x%X)\n", info.adc, info.adc);
    cliPrintf("mBar    : %s %d.%02d\n",
              info.mBar_x100 < 0 ? "-" : "+",
              abs(info.mBar_x100 / 100),
              abs(info.mBar_x100 % 100));
    cliPrintf("psi     : %s %d.%02d\n",
              info.psi_x100 < 0 ? "-" : "+",
              info.psi_x100 / 100,
              abs(info.psi_x100 % 100));

    cliPrintf("mmHg    : %s %d.%02d\n",
              info.mmHg < 0 ? "-" : "+",
              abs((int)(info.mmHg)),
              abs((int)(info.mmHg * 100) % 100));

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") == true)
  {
    us6300_info_t info;
    uint8_t ch = (uint8_t)args->getData(1);

    cliShowCursor(false);
    while (cliKeepLoop())
    {
      us6300GetInfoChannel(ch, &info);
      cliPrintf("adc     : %d (0x%X)\n", info.adc, info.adc);
      cliPrintf("mBar    : %s %d.%02d\n",
                info.mBar_x100 < 0 ? "-" : "+",
                abs(info.mBar_x100 / 100),
                abs(info.mBar_x100 % 100));
      cliPrintf("psi     : %s %d.%02d\n",
                info.psi_x100 < 0 ? "-" : "+",
                info.psi_x100 / 100,
                abs(info.psi_x100 % 100));

      cliPrintf("mmHg    : %s %d.%02d\n",
                info.mmHg < 0 ? "-" : "+",
                abs((int)(info.mmHg)),
                abs((int)(info.mmHg * 100) % 100));

      delay(100);
      cliMoveUp(4);
    }
    cliMoveDown(4);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "all") == true)
  {
    us6300_info_t info[TCA9548_MAX_CH];

    cliShowCursor(false);
    while (cliKeepLoop())
    {
      for (uint8_t ch = 0; ch < TCA9548_MAX_CH; ch++)
      {
        us6300GetInfoChannel(ch, &info[ch]);
        cliPrintf("ch[%d] mBar    : %s %d.%02d\n",
                  ch,
                  info[ch].mBar_x100 < 0 ? "-" : "+",
                  abs(info[ch].mBar_x100 / 100),
                  abs(info[ch].mBar_x100 % 100));
        delay(25);        
      } 
      cliMoveUp(TCA9548_MAX_CH);     
    }
    cliMoveDown(TCA9548_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("us6300 info [0~%d]\n", TCA9548_MAX_CH-1);
    cliPrintf("us6300 show [0~%d]\n", TCA9548_MAX_CH-1);
    cliPrintf("us6300 all\n");
  }
}
#endif

#endif
