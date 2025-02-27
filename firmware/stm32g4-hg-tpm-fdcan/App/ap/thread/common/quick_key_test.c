  #include "quick_key_test.h"

  #include "thread.h"
  #include "event.h"
  #include "manage/can.h"
  #include "manage/msg_queue.h"

  #define DATA_SEND_PERIOD  100 // ms
  #define TEST_TIMEOUT      3000 // ms

  static bool quickKeyTestThreadInit(void);
  static bool quickKeyTestThreadUpdate(void);
  static void quickKeyTestThreadISR(void *arg);

  static uint32_t prev_time = 0;

  static void canSendQuickKeyTestResult(uint8_t result, uint8_t data);

  __attribute__((section(".thread"))) 
  static volatile thread_t thread_obj = 
  {
    .name = "quickKeyTest",
    .is_enable = true,
    .init = quickKeyTestThreadInit,
    .update = quickKeyTestThreadUpdate
  };


  typedef enum
  {
    QUICK_KEY_TEST_IDLE = 0,
    QUICK_KEY_TEST_START,
    QUICK_KEY_TEST_STOP
  } quick_key_test_state_t;

  bool quickKeyTestThreadInit(void)
  {
    swtimer_handle_t timer_ch;
    timer_ch = swtimerGetHandle();
    if (timer_ch >= 0)
    {
      swtimerSet(timer_ch, 10, LOOP_TIME, quickKeyTestThreadISR, NULL);
      swtimerStart(timer_ch);
    }
    else
    {
      logPrintf("[NG] quickKeyTestThreadInit()\n     swtimerGetHandle()\n");
    }
    return true;
  }

  bool quickKeyTestThreadUpdate(void)
  {
    static uint8_t is_test_start = 0;
    static uint8_t quick_key_send_data = 0;
    static uint8_t quick_key_receive_data = 0;
    static uint8_t quick_key_test_result = 0;
    static uint32_t test_start_time = 0;
    static bool is_testing = false;

    queue_msg_t msg;
    
    // 큐에서 메시지 확인
    if (msgQueueGet(&msg))
    {
        if (msg.msg_id == ID_QUICK_KEY_ACTION)
        {
            // data[0]: 테스트 시작 여부 (0: idle, 1: test_start)
            // data[1]: 테스트할 데이터 값 (0~255)
            is_test_start = msg.data[0];
            if (is_test_start == QUICK_KEY_TEST_START)  // 테스트 시작 명령인 경우
            {
                // 테스트 시작
                logPrintf("quick key test start\n");
                quick_key_send_data = msg.data[1];  // 1번 바이트 데이터 저장
                uartWrite(HW_UART_CH_QKEY, &quick_key_send_data, 1);
                test_start_time = millis();
                is_testing = true;
                quick_key_test_result = 0;
                quick_key_receive_data = 0;
            }
            else if (is_test_start == QUICK_KEY_TEST_STOP)  // 테스트 중지 명령인 경우
            {
                // 테스트 중지
                logPrintf("quick key test stop\n");
                is_testing = false;
                quick_key_test_result = 0;
                quick_key_receive_data = 0;
            }
        }
    }

    // 테스트 중이면 응답 확인
    if (is_testing)
    {
        if (uartAvailable(HW_UART_CH_QKEY))
        {
            quick_key_receive_data = uartRead(HW_UART_CH_QKEY);
            
            if (quick_key_send_data == quick_key_receive_data)
            {
                quick_key_test_result = 1;  // 성공
            }
            is_testing = false;  // 테스트 종료
        }
        else if (millis() - test_start_time >= TEST_TIMEOUT)  // 타임아웃 체크
        {
            is_testing = false;  // 테스트 종료
        }
    }
    
    if (millis() - prev_time >= DATA_SEND_PERIOD)
    {
        prev_time = millis();

        if (canObj()->isOpen() == true)
        {
              canSendQuickKeyTestResult(quick_key_test_result, quick_key_receive_data);
        }
    }

    return true;
  }

  void quickKeyTestThreadISR(void *arg)
  {  

  }

  static void canSendQuickKeyTestResult(uint8_t result, uint8_t data)
  {
    static uint8_t tx_cnt = 0;
    can_msg_t msg;
    
    msg.frame   = CAN_CLASSIC;
    msg.id_type = CAN_STD;
    msg.dlc     = CAN_DLC_3;
    msg.id      = ID_MAIN_QUICK_RESULT;
    msg.length  = 3;

    msg.data[0] = tx_cnt++;
    msg.data[1] = (uint8_t)(result);
    msg.data[2] = (uint8_t)(data);

    canMsgWrite(_DEF_CAN1, &msg, 10);
    canHandleTxMessage(&msg);
  }

