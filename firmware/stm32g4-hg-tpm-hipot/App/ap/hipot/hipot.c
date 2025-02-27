// hipot.c

#include "hipot.h"


#ifdef _USE_HW_CLI
static void cliHipot(cli_args_t *args);
#endif

#define HIPOT_BUSY_TIMEOUT  5500 // BUSY 상태 유지 시간 [ms]
#define HIPOT_RETRY_TIMEOUT 1000 // 송신 측 재전송시, 무시하는 시간 [ms]
#define HIPOT_SEND_TIME     100  // 상태 송신 주기 [ms]
#define HIPOT_DELAY_TIME    100  // 송신 딜레이 시간 [ms]
#define DEFAULT_COMMAND     0xFF


static HipotState_t  hipot_state      = HIPOT_STATE_IDLE;
static HipotResult_t hipot_result     = HIPOT_RESULT_NONE;
static uint32_t      hipot_delay_time = HIPOT_DELAY_TIME;
static uint32_t      busy_timeout     = HIPOT_BUSY_TIMEOUT;
static uint8_t       ch               = _DEF_CAN1;
static bool          is_init          = false;
static uint32_t      start_busy_time  = 0;
static uint8_t       last_command     = DEFAULT_COMMAND;
static uint8_t       tx_cnt           = 0;


static bool isHipotResultPass(void);
static bool isHipotResultFail(void);
static void hipotStart(void);
static void hipotStop(void);
static void hipotProcessCommand(void);
static void hipotCheckResult(void);
static void hipotSendStatus(void);
static void hipotCheckTimeout(void);
static void hipotProcessActionCommand(can_msg_t *msg);
static void hipotProcessClearCommand(can_msg_t *msg);

void hipotInit(void)
{
#ifdef _USE_HW_CLI
  cliAdd("hipot", cliHipot);
#endif

  logPrintf("[OK] hipotInit()\n");

  is_init = true;
}

void hipotMain(void)
{
  if (canIsOpen(ch))
  {
    hipotProcessCommand();
    hipotCheckResult();
    hipotCheckTimeout();
    hipotSendStatus();
  }
}

bool isHipotResultPass(void)
{
  return gpioPinRead(P_IN1) == false;
}

bool isHipotResultFail(void)
{
  return gpioPinRead(P_IN2) == false;
}

void hipotStart(void)
{
  gpioPinWrite(P_OUT1, true);
  delay(hipot_delay_time);
  gpioPinWrite(P_OUT1, false);
}

void hipotStop(void)
{
  gpioPinWrite(P_OUT2, true);
  delay(hipot_delay_time);
  gpioPinWrite(P_OUT2, false);
}

void hipotProcessActionCommand(can_msg_t *msg)
{
  static uint32_t last_action_time = 0;

  if (last_command != DEFAULT_COMMAND)
  {
    if (millis() - last_action_time > HIPOT_RETRY_TIMEOUT)
    {
      last_command = DEFAULT_COMMAND;
    }
    else
    {
      return;
    }
  }

  if (msg->id == ID_HIPOT_ACTION && msg->length == 1)
  {
    uint8_t set_value = msg->data[0];
    {
      const char *state[] = {"IDLE", "START", "STOP"};
      logPrintf("hipot action: %s (%d)\n", state[set_value], set_value);

      switch (set_value)
      {
        case HIPOT_SET_IDLE:
          last_command     = HIPOT_SET_IDLE;
          hipot_state      = HIPOT_STATE_IDLE;
          hipot_result     = HIPOT_RESULT_NONE;
          last_action_time = millis();
          logPrintf("hipot idle\n");
          break;
        case HIPOT_SET_START:
          last_command     = HIPOT_SET_START;
          hipot_state      = HIPOT_STATE_BUSY;
          hipot_result     = HIPOT_RESULT_NONE;
          last_action_time = millis();
          start_busy_time  = millis();
          logPrintf("hipot start\n");
          hipotStart();
          break;
        case HIPOT_SET_STOP:
          last_command     = HIPOT_SET_STOP;
          hipot_state      = HIPOT_STATE_PAUSE;
          hipot_result     = HIPOT_RESULT_FAIL;
          last_action_time = millis();
          logPrintf("hipot stop\n");
          hipotStop();
          break;
        default:
          break;
      }
    }
  }
}

void hipotProcessClearCommand(can_msg_t *msg)
{
  if (msg->id == ID_HIPOT_CLEAR && msg->length == 3)
  {
    const char *state[]  = {"IDLE", "BUSY", "PAUSE", "DONE"};
    const char *result[] = {"NONE", "PASS", "FAIL"};

    logPrintf("hipot clear command received\n");

    // tx_cnt clear
    if (msg->data[0] == 1)
    {
      logPrintf("tx_cnt : %d->0\n", tx_cnt);
      tx_cnt = 0;
    }

    // hipot_state clear
    if (msg->data[1] == 1)
    {
      if (hipot_state != HIPOT_STATE_IDLE)
      {
        logPrintf("state  : %s->%s\n", state[hipot_state], state[HIPOT_STATE_IDLE]);
        hipot_state = HIPOT_STATE_IDLE;
      }
    }

    // hipot_result clear
    if (msg->data[2] == 1)
    {
      if (hipot_result != HIPOT_RESULT_NONE)
      {
        logPrintf("result : %s->%s\n", result[hipot_result], result[HIPOT_RESULT_NONE]);
        hipot_result = HIPOT_RESULT_NONE;
      }
    }
  }
}

static void hipotProcessCommand(void)
{
  if (canMsgAvailable(ch))
  {
    can_msg_t msg;
    canMsgRead(ch, &msg);

    hipotProcessActionCommand(&msg);
    hipotProcessClearCommand(&msg);
  }
}

static void hipotCheckResult(void)
{
  if (hipot_state == HIPOT_STATE_BUSY)
  {
    if (isHipotResultPass())
    {
      hipot_result = HIPOT_RESULT_PASS;
      hipot_state  = HIPOT_STATE_DONE;
      logPrintf("hipot result: PASS\n");
    }
    else if (isHipotResultFail())
    {
      hipot_result = HIPOT_RESULT_FAIL;
      hipot_state  = HIPOT_STATE_PAUSE;
      logPrintf("hipot result: FAIL\n");
    }
  }
}

static void hipotSendStatus(void)
{
  static uint32_t pre_time = 0;

  if (millis() - pre_time >= HIPOT_SEND_TIME)
  {
    pre_time = millis();

    can_msg_t msg;
    msg.frame   = CAN_CLASSIC;
    msg.id_type = CAN_STD;
    msg.dlc     = CAN_DLC_3;
    msg.id      = ID_HIPOT_STATE;
    msg.length  = 3;

    msg.data[0] = tx_cnt++;     // 카운트 값 (0~255)
    msg.data[1] = hipot_state;  // HIPOT 상태 (IDLE=0, BUSY=1, PAUSE=2, DONE=3)
    msg.data[2] = hipot_result; // HIPOT 결과 (NONE=0, PASS=1, FAIL=2, TIMEOUT=3)

    canMsgWrite(_DEF_CAN1, &msg, 10);
  }
}

static void hipotCheckTimeout(void)
{
  if (hipot_state == HIPOT_STATE_BUSY)
  {
    if (millis() - start_busy_time >= busy_timeout)
    {
      logPrintf("hipot timeout occurred %d [ms]\n", busy_timeout);
      hipot_state  = HIPOT_STATE_PAUSE;
      hipot_result = HIPOT_RESULT_TIMEOUT;
      last_command = DEFAULT_COMMAND;
      hipotStop();
    }
  }
}

#ifdef _USE_HW_CLI
void cliHipot(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    const char *state[]  = {"IDLE", "BUSY", "PAUSE", "DONE"};
    const char *result[] = {"NONE", "PASS", "FAIL"};

    cliPrintf("hipot info\n");
    cliPrintf("init           : %s\n", is_init ? "OK" : "NG");
    cliPrintf("state          : %s\n", state[hipot_state]);
    cliPrintf("result         : %s\n", result[hipot_result]);
    cliPrintf("tx_cnt         : %d\n", tx_cnt);
    cliPrintf("busy_timeout   : %d [ms]\n", busy_timeout);
    cliPrintf("delay_time     : %d [ms]\n", HIPOT_DELAY_TIME);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "clear") == true)
  {
    const char *state[]  = {"IDLE", "BUSY", "PAUSE", "DONE"};
    const char *result[] = {"NONE", "PASS", "FAIL"};

    cliPrintf("hipot clear\n");

    if (hipot_state != HIPOT_STATE_IDLE)
    {
      cliPrintf("state  : %s->%s\n", state[hipot_state], state[HIPOT_STATE_IDLE]);
      hipot_state = HIPOT_STATE_IDLE;
    }
    else
    {
      cliPrintf("state  : %s\n", state[hipot_state]);
    }

    if (hipot_result != HIPOT_RESULT_NONE)
    {
      cliPrintf("result : %s->%s\n", result[hipot_result], result[HIPOT_RESULT_NONE]);
      hipot_result = HIPOT_RESULT_NONE;
    }
    else
    {
      cliPrintf("result : %s\n", result[hipot_result]);
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "delay") == true)
  {
    uint32_t delay_time;
    delay_time = args->getData(1);
    delay_time = constrain(delay_time, 50, 1000);

    if (hipot_delay_time != delay_time)
    {
      cliPrintf("hipot delay: %d -> %d [ms]\n", hipot_delay_time, delay_time);
      hipot_delay_time = delay_time;
    }
    else
    {
      cliPrintf("hipot delay is %d [ms], not changed\n", hipot_delay_time);
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "timeout") == true)
  {
    uint32_t timeout_time;
    timeout_time = args->getData(1);
    timeout_time = constrain(timeout_time, 1000, 60000);

    if (busy_timeout != timeout_time)
    {
      cliPrintf("hipot timeout: %d -> %d [ms]\n", busy_timeout, timeout_time);
      busy_timeout = timeout_time;
    }
    else
    {
      cliPrintf("hipot timeout is %d [ms], not changed\n", busy_timeout);
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "start") == true)
  {
    cliPrintf("hipot start\n");
    hipotStart();
    ret = true;
  }
  else if (args->argc == 1 && args->isStr(0, "stop") == true)
  {
    cliPrintf("hipot stop\n");
    hipotStop();

    ret = true;
  }

  else if (args->argc == 1 && args->isStr(0, "stat") == true)
  {
    while (cliKeepLoop())
    {
      if (gpioPinRead(P_IN1) == false)
      {
        cliPrintf("hipot status: PASS\n");
        break;
      }
      if (gpioPinRead(P_IN2) == false)
      {
        cliPrintf("hipot status: FAIL\n");
        break;
      }

      delay(100);
    }

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("hipot info\n");
    cliPrintf("hipot delay [50 ~ 1000] ms\n");
    cliPrintf("hipot timeout [1000 ~ 60000] ms\n");
    cliPrintf("hipot clear\n");
    cliPrintf("hipot start\n");
    cliPrintf("hipot stop\n");
    cliPrintf("hipot stat\n");
  }
}
#endif
