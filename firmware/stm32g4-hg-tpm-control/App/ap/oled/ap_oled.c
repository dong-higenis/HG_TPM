#include "ap_oled.h"
#include "can.h"
#include "lcd.h"
#include "gpio.h"

#define OLED_UPDATE_INTERVAL 500
#define LED_UPDATE_INTERVAL  500

static uint8_t ch = _DEF_CAN1;

typedef enum
{
  MODE_NONE = 0,
  MODE_BARCODE,
  MODE_INTERNAL_VOLTAGE,
  MODE_PRESSURE_SENSOR,
  MODE_SAFETY_SENSOR,
  MODE_LTM,
  MODE_RESULT,
  MODE_EMERGENCY_STOP,
  MODE_NO_PROFILE,
  MODE_BOARD_CHECK_CONNECT,
  MODE_MAX
} mode_t;

typedef enum
{
  STATE_NONE = 0,
  STATE_WAITING,
  STATE_TESTING,
  STATE_COMPLETED,
  STATE_FAILED,
  STATE_OK,
  STATE_NG,
  STATE_MAX
} state_t;

static const char *const mode_str[] = {
  "None",
  "바코드",
  "내전압",
  "공압센서",
  "안전센서",
  "LTM",
  "검사결과",
  "비상정지",
  "프로필 없음",
  "보드 연결 확인"
};

static const char *const state_str[] = {
  "None",
  "대기중",
  "검사중",
  "완료",
  "실패",
  "OK",
  "NG"};

static uint32_t prev_time  = 0;    // 갱신 시간 체크용

static uint8_t mode  = 0;
static uint8_t state = 0;
static uint8_t safety_sensor_value = 0 ;

//
//  <<-- 안전센서 추가 코드
//

#define SAFETY_SENSOR_COUNT 4 // safety sensor 개수
#define BOX_WIDE_SIZE 30  // 안전센서 박스 wide 크기
#define STRING_DATA_OLED_Y_LOCATION 16 // 안전센서 정보 출력 시 모든 글자는 OLED의 y좌표 16에서 시작함 

enum
{
  led_green = 0,
  led_blue,
  led_red,
};

enum
{
  led_off = 0,
  led_on,
  led_toggle,
};

//안전센서 활성 여부 디스플레이 문자의 x축 시작점
static const uint8_t box_start_location[SAFETY_SENSOR_COUNT] = { 131, 162, 193, 224 };
//  각 안전센서 번호 디스플레이 문자
static const char *const box_string[] = { "1", "2", "3", "4" };     

static const uint8_t safety_mask[SAFETY_SENSOR_COUNT] = {0x8, 0x1, 0x2, 0x4};

//
//  -->> 안전센서 추가 코드
//

static uint8_t led_state[3] = {0x00,};

void apOledProcessCmd(void);
void buttonLedProcess(void);

void displaySafetySensor(uint8_t top_line, uint8_t bot_line, uint8_t lef_line, const uint8_t display_string_y_location, uint8_t display_draw_number_flag, const char *str);

static void cliOled(cli_args_t *args);

void apOledInit(void)
{
  logPrintf("[OK] apOledInit()\n");
  cliAdd("oled", cliOled);
}

void apOledMain(void)
{
  if (canIsOpen(ch))
  {
    apOledProcessCmd();
    buttonLedProcess();
  }
}

void apOledProcessCmd(void)
{
  can_msg_t rx_msg;

  if (canMsgAvailable(ch))
  {
    canMsgRead(ch, &rx_msg);

    if (rx_msg.id == ID_MODE_AND_STATE ) // --> 길이 제한 임의 해제
    //if (rx_msg.id == ID_MODE_AND_STATE && rx_msg.length == 2)
    {
      mode  = rx_msg.data[0];
      state = rx_msg.data[1];
      led_state[led_green] = rx_msg.data[3];
      led_state[led_blue] = rx_msg.data[4];
      led_state[led_red] = rx_msg.data[5];

      if (millis() - prev_time > OLED_UPDATE_INTERVAL)
      {
        prev_time  = millis();

        if (lcdDrawAvailable())
        {
          // 범위 체크
          mode  = constrain(mode, MODE_NONE, MODE_MAX - 1);
          state = constrain(state, STATE_NONE, STATE_MAX -1);

          if (mode == MODE_NONE )//|| state == STATE_NONE)
          {
            // 모드나 상태가 0이면 화면을 클리어만 함
            lcdClearBuffer(black);
            lcdRequestDraw();
            logPrintf("Display Clear\n");
          }
          //
          //  <<-- 안전센서 UI 관련 코드 시작점
          //
          else if ( mode == MODE_SAFETY_SENSOR && state == STATE_TESTING )
          {
            safety_sensor_value = rx_msg.data[2];

            uint8_t safety_sensor_value_tmp = 0 ;

            uint8_t top_line = 11;
            uint8_t bottom_line = 50;

            char display_buffer[32] = {0x00,};
            uint8_t display_draw_number_flag[SAFETY_SENSOR_COUNT] = {0x00,};

            for(int i = 0 ; i < SAFETY_SENSOR_COUNT ; i++ ) //  4개의 상태값만 받을 예정
            {
              safety_sensor_value_tmp = safety_sensor_value & safety_mask[i];
              if(safety_sensor_value_tmp == safety_mask[i] )
              {
                display_draw_number_flag[i] = on;
              }
              else
              {
                display_draw_number_flag[i] = off;
              }
            }

            snprintf(display_buffer, sizeof(display_buffer), "%s", mode_str[MODE_SAFETY_SENSOR]);

            lcdClearBuffer(black);

            lcdPrintf( 1, STRING_DATA_OLED_Y_LOCATION, white, display_buffer);

            for(int i = 0 ; i < SAFETY_SENSOR_COUNT ; i++)  //  안전센서 활성 여부 박스 그리기
            {
              displaySafetySensor(top_line, bottom_line, box_start_location[i], STRING_DATA_OLED_Y_LOCATION, display_draw_number_flag[i], box_string[i]);
            }

            //  최종 버퍼 그리기
            lcdRequestDraw();
          }
          //
          //  -->>
          //
          else if (( mode == MODE_EMERGENCY_STOP) || ( mode == MODE_NO_PROFILE) || ( mode == MODE_BOARD_CHECK_CONNECT))
          {
            char str[32];            // 화면 갱신
            snprintf(str, sizeof(str), "%s", mode_str[mode]);
            lcdClearBuffer(black);
            lcdPrintf((LCD_WIDTH - getStringWidth(str)) / 2, 16, white, str);
            lcdRequestDraw();
            logPrintf("%s\n", str); // 로그 출력
          }
          else
          {
            // 문자열 조합
            char str[32];
#if 0
            if (( mode == MODE_EMERGENCY_STOP) || ( mode == MODE_NO_PROFILE) || ( mode == MODE_BOARD_CHECK_CONNECT))
              snprintf(str, sizeof(str), "%s", mode_str[mode]);
            else
              snprintf(str, sizeof(str), "%s %s", mode_str[mode], state_str[state]);
#else
            snprintf(str, sizeof(str), "%s %s", mode_str[mode], state_str[state]);
#endif
            // 화면 갱신
            lcdClearBuffer(black);
            lcdPrintf((LCD_WIDTH - getStringWidth(str)) / 2, 16, white, str);
            lcdRequestDraw();
            logPrintf("%s\n", str); // 로그 출력
          }                   
        }
      }
    }
  }
}

// 검사 모드와 상태에 따른 버튼의 LED 제어
void buttonLedProcess(void)
{
  static uint32_t prev_time = 0;
  static bool toggle_flag = false;

  if (millis() - prev_time > LED_UPDATE_INTERVAL)
  {
    prev_time = millis();

    toggle_flag ^= 1; // toggle 동기화

    for(int i = 0 ; i < 3 ; i++ )
    {
      switch(led_state[i])
      {
        case led_off:
          gpioPinWrite(GPIO_CH_P_LED1 + i, _DEF_LOW);
          break;

        case led_on:
          gpioPinWrite(GPIO_CH_P_LED1 + i, _DEF_HIGH);
          break;

        case led_toggle:
          gpioPinWrite(GPIO_CH_P_LED1 + i, toggle_flag);
          break;

        default:
          break;
      }
    }
  }
}

void displaySafetySensor(uint8_t top_line, uint8_t bottom_line, uint8_t left_line, const uint8_t display_string_y_location, uint8_t display_draw_number_flag, const char *str)
{
  uint8_t right_line = left_line + BOX_WIDE_SIZE;
  
  lcdDrawFillRect(left_line, top_line, right_line - left_line, bottom_line - top_line, white );
  lcdDrawFillRect(left_line + 1, top_line + 1, right_line - left_line - 2, bottom_line - top_line - 2, black);

  if(display_draw_number_flag == off)
  {
    lcdPrintf(left_line + 6, display_string_y_location, white, str);
  }
  else if(display_draw_number_flag == on)
  {
    lcdDrawFillRect(left_line + 2, top_line + 2, right_line - left_line - 4, bottom_line - top_line - 4, white);
    lcdPrintf(left_line + 6, display_string_y_location, black, str);
  }
}


void cliOled(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("cliOled init\n");
    ret = true;
  }
  else if (args->argc == 2 && args->isStr(0, "display_center_char") == true)
  {
    int index = args->getData(1);
    // 문자열 조합
    char str[32];

    switch(index)
    {
      case 0:
        index = MODE_EMERGENCY_STOP;
        break;

      case 1:
        index = MODE_NO_PROFILE;
        break;

      case 2:
        index = MODE_BOARD_CHECK_CONNECT;
        break;
    }

    snprintf(str, sizeof(str), "%s", mode_str[index]);

    // 화면 갱신
    lcdClearBuffer(black);
    lcdPrintf((LCD_WIDTH - getStringWidth(str)) / 2, 16, white, str);
    lcdRequestDraw();
    logPrintf("%s\n", str); // 로그 출력
  }
  else if (args->argc == 1 && args->isStr(0, "test1") == true)
  {
    char str[32];
    snprintf(str, sizeof(str), "%s %s", mode_str[MODE_SAFETY_SENSOR], state_str[STATE_WAITING]);

    // 화면 갱신
    lcdClearBuffer(black);
    lcdPrintf((LCD_WIDTH - getStringWidth(str)) / 2, 16, white, str);
    lcdRequestDraw();
    logPrintf("%s\n", str); // 로그 출력
  }
  else if (args->argc == 2 && args->isStr(0, "test2") == true)
  {
    safety_sensor_value = args->getData(1);

    uint8_t safety_sensor_value_tmp = 0 ;

    uint8_t top_line = 11;
    uint8_t bottom_line = 50;

    char display_buffer[32] = {0x00,};
    uint8_t display_draw_number_flag[SAFETY_SENSOR_COUNT] = {0x00,};

    for(int i = 0 ; i < SAFETY_SENSOR_COUNT ; i++ ) //  4개의 상태값만 받을 예정
    {
      safety_sensor_value_tmp = safety_sensor_value & safety_mask[i];
      if(safety_sensor_value_tmp == safety_mask[i] )
      {
        display_draw_number_flag[i] = on;
      }
      else
      {
        display_draw_number_flag[i] = off;
      }
    }

    snprintf(display_buffer, sizeof(display_buffer), "%s", mode_str[MODE_SAFETY_SENSOR]);

    lcdClearBuffer(black);

    lcdPrintf( 1, STRING_DATA_OLED_Y_LOCATION, white, display_buffer);

    for(int i = 0 ; i < SAFETY_SENSOR_COUNT ; i++)  //  안전센서 활성 여부 박스 그리기
    {
      displaySafetySensor(top_line, bottom_line, box_start_location[i], STRING_DATA_OLED_Y_LOCATION, display_draw_number_flag[i], box_string[i]);
    }

    //  최종 버퍼 그리기
    lcdRequestDraw();

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("oled info\n");
  }
}

