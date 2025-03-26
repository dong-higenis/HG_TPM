#include "signtower.h"

#ifdef _USE_HW_SIGNTOWER
static void cliSignTower(cli_args_t *args);
#endif

#define SIGN_TOWER_SEND_TIME 100 // 상태 송신 주기 [ms]

typedef struct
{
  uint8_t color;
  bool    is_blink;
  bool    is_buzzer_on;
} sign_tower_t;


static uint8_t ch      = _DEF_CAN1;
static bool    is_init = false;
static uint8_t tx_cnt  = 0;

static sign_tower_t sign_tower;

static void signtowerProcessCommand(void);
static void signtowerSendStatus(void);
static void signtowerSetCommand(can_msg_t *msg);
static void signtowerSetColor(uint8_t color);
static void signtowerSetBlink(uint8_t is_blink);
static void signtowerSetBuzzer(uint8_t is_on);

void signtowerInit(void)
{
#ifdef _USE_HW_CLI
  cliAdd("signtower", cliSignTower);
#endif

  logPrintf("[OK] signtowerInit()\n");

  sign_tower.color        = SIGNTOWER_COLOR_OFF;
  sign_tower.is_blink     = false;
  sign_tower.is_buzzer_on = false;

  is_init = true;
}

void signtowerMain(void)
{
  if (canIsOpen(ch))
  {
    signtowerProcessCommand();
    signtowerSendStatus();
  }
}

void signtowerProcessCommand(void)
{
  if (canMsgAvailable(ch))
  {
    can_msg_t msg;
    canMsgRead(ch, &msg);

    signtowerSetCommand(&msg);
  }
}

void signtowerSendStatus(void)
{
  static uint32_t pre_time = 0;

  if (millis() - pre_time >= SIGN_TOWER_SEND_TIME)
  {
    pre_time = millis();

    can_msg_t msg;
    msg.frame   = CAN_CLASSIC;
    msg.id_type = CAN_STD;
    msg.dlc     = CAN_DLC_4;
    msg.id      = ID_SIGNTOWER_STATE;
    msg.length  = 4;

    msg.data[0] = tx_cnt++;     // 카운트 값 (0~255)
    msg.data[1] = sign_tower.color;
    msg.data[2] = sign_tower.is_blink;
    msg.data[3] = sign_tower.is_buzzer_on;

    canMsgWrite(_DEF_CAN1, &msg, 10);
  }
}

void signtowerSetCommand(can_msg_t *msg)
{
  if (msg->id == ID_SIGNTOWER_LED_SET && msg->length == 2)
  {
    uint8_t color        = msg->data[0];
    uint8_t is_blink     = msg->data[1];

    sign_tower.color        = color;
    sign_tower.is_blink     = is_blink;

    signtowerSetColor(color);
    signtowerSetBlink(is_blink);
  }

  if (msg->id == ID_SIGNTOWER_BUZZER_SET && msg->length == 1)
  {
    uint8_t is_buzzer_on = msg->data[0];

    sign_tower.is_buzzer_on = is_buzzer_on;

    signtowerSetBuzzer(is_buzzer_on);
  }
}

void signtowerSetColor(uint8_t color)
{
  switch (color)
  {
    case SIGNTOWER_COLOR_OFF:
      gpioPinWrite(P_OUT1, _DEF_LOW);
      gpioPinWrite(P_OUT2, _DEF_LOW);
      gpioPinWrite(P_OUT3, _DEF_LOW);
      break;
    case SIGNTOWER_COLOR_RED:
      gpioPinWrite(P_OUT1, _DEF_HIGH);
      gpioPinWrite(P_OUT2, _DEF_LOW);
      gpioPinWrite(P_OUT3, _DEF_LOW);
      break;
    case SIGNTOWER_COLOR_YELLOW:
      gpioPinWrite(P_OUT1, _DEF_LOW);
      gpioPinWrite(P_OUT2, _DEF_HIGH);
      gpioPinWrite(P_OUT3, _DEF_LOW);
      break;
    case SIGNTOWER_COLOR_GREEN:
      gpioPinWrite(P_OUT1, _DEF_LOW);
      gpioPinWrite(P_OUT2, _DEF_LOW);
      gpioPinWrite(P_OUT3, _DEF_HIGH);
      break;
  }
}

void signtowerSetBlink(uint8_t is_blink)
{
  if (is_blink)
  {
    gpioPinWrite(P_OUT8, _DEF_HIGH);
  }
  else
  {
    gpioPinWrite(P_OUT8, _DEF_LOW);
  }
}

void signtowerSetBuzzer(uint8_t is_on)
{
  if (is_on)
  {
    gpioPinWrite(P_OUT5, _DEF_HIGH);
  }
  else
  {
    gpioPinWrite(P_OUT5, _DEF_LOW);
  }
}


#ifdef _USE_HW_CLI
void cliSignTower(cli_args_t *args)
{
  bool ret = false;
  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("signtower init %s\n", is_init ? "true" : "false");
    cliPrintf("signtower color %d\n", sign_tower.color);
    cliPrintf("signtower blink %d\n", sign_tower.is_blink);
    cliPrintf("signtower buzzer %d\n", sign_tower.is_buzzer_on);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "color") == true)
  {
    cliPrintf("signtower set color\n");

    uint8_t color = args->getData(1);
    sign_tower.color = color;
    signtowerSetColor(color);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "blink") == true)
  {
    cliPrintf("signtower blink\n");

    uint8_t is_on = args->getData(1);
    sign_tower.is_blink = is_on;
    signtowerSetBlink(is_on);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "buzzer") == true)
  {
    cliPrintf("signtower buzzer\n");

    uint8_t is_on = args->getData(1);
    sign_tower.is_buzzer_on = is_on;
    signtowerSetBuzzer(is_on);

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("signtower info\n");
    cliPrintf("signtower color [color 0 ~ 3 off|red|yellow|green]\n");
    cliPrintf("signtower blink [is_blink 0 ~ 1 off|on]\n");
    cliPrintf("signtower buzzer [is_on 0 ~ 1 off|on]\n");
  }
}
#endif
