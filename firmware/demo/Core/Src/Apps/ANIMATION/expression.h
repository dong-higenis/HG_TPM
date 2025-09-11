#include "oled_eyes.h"
#include "main.h"

typedef enum {
    EYE_STATE_NORMAL,
    EYE_STATE_HAPPY,
    EYE_STATE_SAD,
    EYE_STATE_SURPRISED,
    EYE_STATE_BLINK,
    EYE_STATE_MOVE_LEFT,
    EYE_STATE_MOVE_RIGHT,
    EYE_STATE_SLEEP
} eye_state_t;

// 전역 변수들
static eye_state_t current_eye_state = EYE_STATE_BLINK;
static uint32_t last_expression_time = 0;
static uint32_t expression_duration = 100;
