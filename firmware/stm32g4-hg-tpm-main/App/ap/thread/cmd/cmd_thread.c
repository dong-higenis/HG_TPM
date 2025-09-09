#include "cmd_thread.h"

#include "thread.h"
#include "manage/mode.h"
#include "driver/cmd_uart.h"



enum
{
  DRIVER_CH_USB = 0,
  DRIVER_CH_RS232,
  DRIVER_CH_MAX
};



#define CMD_DRIVER_MAX_CH     DRIVER_CH_MAX


typedef struct
{
  int32_t count;
  cmd_process_t *p_process;
} cmd_process_info_t;

static bool cmdThreadInit(void);
static bool cmdThreadUpdate(void);

static cmd_t        cmd[CMD_DRIVER_MAX_CH];
static cmd_driver_t cmd_drvier[CMD_DRIVER_MAX_CH];

static cmd_process_info_t cmd_process_info;
extern uint32_t   _scmd_process;
extern uint32_t   _ecmd_process;
static uint8_t    driver_ch = DRIVER_CH_USB;


__attribute__((section(".thread"))) 
static volatile thread_t thread_obj = 
{
  .name = "cmd",
  .is_enable = true,
  .init = cmdThreadInit,
  .update = cmdThreadUpdate
};

bool cmdThreadInit(void)
{
  (void)thread_obj;

  cmd_process_info.count = ((int)&_ecmd_process - (int)&_scmd_process)/sizeof(cmd_process_t);
  cmd_process_info.p_process = (cmd_process_t *)&_scmd_process;

  logPrintf("[ ] cmdThreadInit()\n");
  logPrintf("    count : %d\n", cmd_process_info.count);
  for (int i=0; i<cmd_process_info.count; i++)
  {
    logPrintf("    %d %s\n", i, cmd_process_info.p_process[i].name);
  }

  cmdUartInitDriver(&cmd_drvier[DRIVER_CH_USB], HW_UART_CH_USB, 1000000);  
  cmdInit(&cmd[DRIVER_CH_USB], &cmd_drvier[DRIVER_CH_USB]);
  cmdOpen(&cmd[DRIVER_CH_USB]);

  cmdUartInitDriver(&cmd_drvier[DRIVER_CH_RS232], HW_UART_CH_RS232, 115200);  
  cmdInit(&cmd[DRIVER_CH_RS232], &cmd_drvier[DRIVER_CH_RS232]);
  cmdOpen(&cmd[DRIVER_CH_RS232]);

  return true;
}

bool cmdThreadUpdate(void)
{

  if (modeObj()->getType() != TYPE_USB_PACKET)
  {
    return false;
  }

  for (int i=0; i<CMD_DRIVER_MAX_CH; i++)
  {
    if (cmd[i].is_init == true)
    {
      if (cmdReceivePacket(&cmd[i]) == true)
      {
        bool ret = false;

        driver_ch = i;

        for (int cnt=0; cnt<cmd_process_info.count; cnt++)
        {
          cmd_process_t *p_process = &cmd_process_info.p_process[cnt];

          if (p_process->cmd_code == cmd[i].packet.cmd)
          {
            if (p_process->cmd_type == cmd[i].packet.type)
            {
              ret |= p_process->process(&cmd[i]);
            }
          }
        }

        if (ret != true && cmd[i].packet.type == PKT_TYPE_CMD)
        {
          cmdSendResp(&cmd[i], cmd[i].packet.cmd, ERR_CMD_NO_CMD, NULL, 0);
        }
      }
    }
  }

  return true;
}

static bool sendPacket(CmdType_t type, uint16_t cmd_code, uint16_t err_code, uint8_t *p_data, uint32_t length)
{
  return cmdSend(&cmd[driver_ch], type, cmd_code, err_code, p_data, length);
}

static bool sendResp(cmd_t *p_cmd, uint16_t err_code, uint8_t *p_data, uint32_t length)
{
  return cmdSendResp(p_cmd, p_cmd->packet.cmd, err_code, p_data, length);
}

cmd_obj_t *cmdObj(void)
{
  static cmd_obj_t cmd_obj = 
  {
    .sendPacket = sendPacket,
    .sendResp = sendResp
  };

  return &cmd_obj;
}
