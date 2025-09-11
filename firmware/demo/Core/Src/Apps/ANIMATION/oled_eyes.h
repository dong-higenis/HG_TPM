#ifndef OLED_EYES_H
#define OLED_EYES_H

#include "ap_oled.h"
#include <stdlib.h>
#include <stdbool.h>

// 256x64 화면용 상수 (원본 128x64에서 2배 확대)
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 64

// Reference Eye 파라미터
#define REF_EYE_HEIGHT 30
#define REF_EYE_WIDTH 50
#define REF_SPACE_BETWEEN_EYE 30
#define REF_CORNER_RADIUS 10

// 색상 정의
#define COLOR_WHITE 0x0F
#define COLOR_BLACK 0x00

// 전역 상태 변수들
extern int left_eye_height, left_eye_width, left_eye_x, left_eye_y;
extern int right_eye_x, right_eye_y, right_eye_height, right_eye_width;
extern int corner_radius;

// 기본 함수들
void eyesInit(void);
void drawEyes(bool update);
void resetEyes(bool update);

// 애니메이션 함수들
void eyesBlink(int speed);
void eyesSleep(void);
void eyesWakeup(void);
void eyesHappy(void);
void eyesSaccade(int direction_x, int direction_y);
void eyesMoveBigEye(int direction);
void eyesMoveRightBigEye(void);
void eyesMoveLeftBigEye(void);

// 애니메이션 제어
void eyesLaunchAnimation(int animation_index);
void eyesSadAnimation(void);

#endif
