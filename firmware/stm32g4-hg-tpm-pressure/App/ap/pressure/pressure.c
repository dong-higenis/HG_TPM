#include "pressure.h"

#define PRESSURE_SEND_TIME    100
#define PRESSURE_MEASURE_TIME 100
#define PRESSURE_MAX_CH       HW_US6300_MAX_CH
#define PRESSURE_MIN_VALUE    0
#define PRESSURE_MAX_VALUE    1023

#define PRESSURE_VERSION_NUMBER 0x00000001


typedef struct
{
  uint16_t crc;
} pressure_tag_t;

typedef struct
{  
  uint32_t magic;
  uint32_t version;
  int32_t  offset[PRESSURE_MAX_CH];
} pressure_cfg_t;


#ifdef _USE_HW_PRESSURE
static void cliPressure(cli_args_t *args);
#endif

static void pressurePowerOn(void);
static void pressurePowerOff(void);
static void pressureSendStatus(void);
static void pressureCheck(void);
static bool pressureLoadCfg(void);

static uint8_t ch      = _DEF_CAN1;
static bool    is_init = false;
static uint8_t tx_cnt  = 0;

static us6300_info_t  info[PRESSURE_MAX_CH];
static pressure_cfg_t cfg;

void pressureInit(void)
{  
#ifdef _USE_HW_CLI
  cliAdd("pressure", cliPressure);
#endif

  logPrintf("[OK] pressureInit()\n");

  pressurePowerOn();

  cfg.magic     = FLASH_MAGIC_NUMBER;
  cfg.version   = PRESSURE_VERSION_NUMBER;
  cfg.offset[0] = 0;
  cfg.offset[1] = 0;
  cfg.offset[2] = 0;
  cfg.offset[3] = 0;

  if (pressureLoadCfg() == true)
  {
    logPrintf("[OK] pressureLoadCfg()\n");
    logPrintf("     magic   : %08x\n", cfg.magic);
    logPrintf("     version : %08x\n", cfg.version);
    logPrintf("     offset  : %d %d %d %d\n", cfg.offset[0], cfg.offset[1], cfg.offset[2], cfg.offset[3]);
  }
  else
  {
    logPrintf("[NG] pressureLoadCfg()\n");
  }

  is_init = true;
}

void pressureMain(void)
{
  pressureCheck();

  if (canIsOpen(ch))
  {
    pressureSendStatus();
  }
}

void pressureCheck(void)
{
  static uint32_t pre_time = 0;
  us6300_info_t info_tmp;

  if (millis() - pre_time >= PRESSURE_MEASURE_TIME)
  {
    pre_time = millis();

    for (uint8_t i = 0; i < PRESSURE_MAX_CH; i++)
    {
      us6300GetInfoChannel(i, &info_tmp);
      info[i].mBar_x100 = info_tmp.mBar_x100 - cfg.offset[i];
    }
  }
}

void pressurePowerOn(void)
{
  gpioPinWrite(mGPIO_OUT1, _DEF_HIGH);
}

void pressurePowerOff(void)
{
  gpioPinWrite(mGPIO_OUT1, _DEF_LOW);
}

void pressureSendStatus(void)
{
  static uint32_t pre_time = 0;

  if (millis() - pre_time >= PRESSURE_SEND_TIME)
  {
    pre_time = millis();

    can_msg_t msg;
    msg.frame   = CAN_CLASSIC;
    msg.id_type = CAN_STD;
    msg.dlc     = CAN_DLC_8;
    msg.id      = ID_PRESSURE_STATE;
    msg.length  = 8;

    msg.data[0] = tx_cnt++; // 카운트 값 (0~255)

    // 스위치 상태를 니블 단위로 설정
    uint8_t switch_status_high = 0;
    uint8_t switch_status_low  = 0;

    // 각 스위치 상태 확인
    if (!gpioPinRead(mGPIO_IN1)) switch_status_low |= 0x10;  // 0번 스위치
    if (!gpioPinRead(mGPIO_IN2)) switch_status_low |= 0x01;  // 1번 스위치
    if (!gpioPinRead(mGPIO_IN3)) switch_status_high |= 0x10; // 2번 스위치
    if (!gpioPinRead(mGPIO_IN4)) switch_status_high |= 0x01; // 3번 스위치

    msg.data[1] = switch_status_low;                         // 0, 1번 스위치 상태
    msg.data[2] = switch_status_high;                        // 2, 3번 스위치 상태

    uint8_t two_msb_bits = 0;

    for (uint8_t i = 0; i < PRESSURE_MAX_CH; i++)
    {
      uint32_t pressure = constrain(info[i].mBar_x100 / 100, PRESSURE_MIN_VALUE, PRESSURE_MAX_VALUE);

      two_msb_bits |= ((pressure >> 8) & 0x03) << (i * 2);

      msg.data[4 + i] = pressure & 0xFF;
    }

    msg.data[3] = two_msb_bits;

    canMsgWrite(_DEF_CAN1, &msg, 10);
  }
}

bool pressureReadCfg(pressure_cfg_t *p_cfg)
{
  bool ret       = false;
  bool flash_ret = true;

  pressure_tag_t tag;
  uint16_t crc_calc;
  uint16_t crc_flash;

  flash_ret &= flashRead(FLASH_CFG_ADDR, (uint8_t *)p_cfg, sizeof(pressure_cfg_t));
  crc_calc = utilCalcCRC(0, (uint8_t *)p_cfg, sizeof(pressure_cfg_t));

  flash_ret &= flashRead(FLASH_CFG_TAG_ADDR, (uint8_t *)&tag, sizeof(pressure_tag_t));
  crc_flash = tag.crc;

  if (flash_ret == true)
  {
    if (p_cfg->magic == FLASH_MAGIC_NUMBER &&  crc_calc == crc_flash)
    {
      ret = true;
    }
  }

  return ret;
}

bool pressureWriteCfg(pressure_cfg_t *p_cfg)
{
  bool ret       = false;
  bool flash_ret = false;
  pressure_tag_t tag;

  flash_ret = flashErase(FLASH_CFG_ADDR, sizeof(pressure_cfg_t));

  if (flash_ret == true)
  {
    flash_ret = flashWrite(FLASH_CFG_ADDR, (uint8_t *)p_cfg, sizeof(pressure_cfg_t));

    if (flash_ret == true)
    {
      tag.crc = utilCalcCRC(0, (uint8_t *)p_cfg, sizeof(pressure_cfg_t));

      flash_ret &= flashErase(FLASH_CFG_TAG_ADDR, sizeof(pressure_tag_t));
      flash_ret &= flashWrite(FLASH_CFG_TAG_ADDR, (uint8_t *)&tag, sizeof(pressure_tag_t));

      if (flash_ret == true)
      {
        ret = true;
      }
    }
  }

  return ret;
}

bool pressureLoadCfg(void)
{
  bool ret = false;
  pressure_cfg_t tmp_cfg;

  ret = pressureReadCfg(&tmp_cfg);
  if (ret == true)
  {
    cfg = tmp_cfg;
  }

  return ret;
}

#ifdef _USE_HW_CLI
void cliPressure(cli_args_t *args)
{
  bool ret = false;
  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("pressure init %s\n", is_init ? "true" : "false");
    cliPrintf("pressure power %s\n", gpioPinRead(mGPIO_OUT1) ? "on" : "off");
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    uint32_t pre_time   = 0;
    uint32_t check_time = 500;

    cliShowCursor(false);
    while (cliKeepLoop())
    {
      pressureCheck();
      if (millis() - pre_time >= check_time)
      {
        pre_time = millis();

        for (uint8_t ch = 0; ch < PRESSURE_MAX_CH; ch++)
        {
          cliPrintf("ch[%d] mBar : %s %3d.%01d\n",
                    ch,
                    info[ch].mBar_x100 < 0 ? "-" : "+",
                    abs(info[ch].mBar_x100 / 100),
                    abs(info[ch].mBar_x100 % 100));
        }
        cliMoveUp(PRESSURE_MAX_CH);
      }
    }
    cliMoveDown(PRESSURE_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "power") == true)
  {
    if (args->isStr(1, "on") == true)
    {
      cliPrintf("pressure power on\n");
      pressurePowerOn();
    }
    else if (args->isStr(1, "off") == true)
    {
      cliPrintf("pressure power off\n");
      pressurePowerOff();
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "info_cfg") == true)
  {
    cliPrintf("magic   : %08x\n", cfg.magic);
    cliPrintf("version : %08x\n", cfg.version);
    cliPrintf("offset : %d %d %d %d\n", cfg.offset[0], cfg.offset[1], cfg.offset[2], cfg.offset[3]);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "read_cfg") == true)
  {
    bool cfg_ret = false;
    cliPrintf("pressure read_cfg\n");
    cfg_ret = pressureReadCfg(&cfg);

    if (cfg_ret == true)
    {
      cliPrintf("magic : %08x\n", cfg.magic);
      cliPrintf("version : %08x\n", cfg.version);
      cliPrintf("offset : %d %d %d %d\n", cfg.offset[0], cfg.offset[1], cfg.offset[2], cfg.offset[3]);
    }
    else
    {
      cliPrintf("pressure read_cfg error\n");
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "write_cfg") == true)
  {
    bool cfg_ret = false;
    cliPrintf("pressure write_cfg\n");

    cfg_ret = pressureWriteCfg(&cfg);
    if (cfg_ret == true)
    {
      cliPrintf("pressure write_cfg success\n");
    }
    else
    {
      cliPrintf("pressure write_cfg error\n");
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "cali_offset") == true)
  {
    cliPrintf("pressure cali_offset\n");

    int32_t offset_sum[PRESSURE_MAX_CH] = {0, };
    us6300_info_t info_tmp;
    pressure_cfg_t tmp_cfg;


    for (uint8_t i = 0; i < 10; i++)
    {
      for (uint8_t ch = 0; ch < PRESSURE_MAX_CH; ch++)
      {
        us6300GetInfoChannel(ch, &info_tmp);
        offset_sum[ch] += info_tmp.mBar_x100;
      }
      delay(100);
    }

    for (uint8_t ch = 0; ch < PRESSURE_MAX_CH; ch++)
    {
      tmp_cfg.offset[ch] = offset_sum[ch] / 10;
    }

    tmp_cfg.magic = FLASH_MAGIC_NUMBER;
    tmp_cfg.version = PRESSURE_VERSION_NUMBER;

    if (pressureWriteCfg(&tmp_cfg) == true)
    {
      if (pressureLoadCfg() == true)
      {
        cliPrintf("pressure cali_offset success\n");
        cliPrintf("magic   : %08x\n", cfg.magic);
        cliPrintf("version : %08x\n", cfg.version);
        cliPrintf("offset  : %d %d %d %d\n", cfg.offset[0], cfg.offset[1], cfg.offset[2], cfg.offset[3]);
      }
      else
      {
        cliPrintf("pressure load_cfg error\n");
      }
    }
    else
    {
      cliPrintf("pressure cali_offset error\n");
    }

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("pressure info\n");
    cliPrintf("pressure show\n");
    cliPrintf("pressure power [on ~ off]\n");
    cliPrintf("pressure info_cfg\n");
    cliPrintf("pressure read_cfg\n");
    cliPrintf("pressure write_cfg\n");
    cliPrintf("pressure cali_offset\n");
  }
}
#endif
