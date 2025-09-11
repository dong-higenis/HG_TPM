#include "expression.h"

// 부드러운 전환을 위한 보간 함수
int lerp(int start, int end, float t)
{
    return start + (int)((end - start) * t);
}

// 현재 상태에서 새로운 상태로 부드럽게 전환
void eyesSmoothTransition(eye_state_t target_state)
{
    // 같은 상태면 전환하지 않음
    if (current_eye_state == target_state)
    {
        return;
    }

    // 현재 눈 상태 저장
    int start_width = left_eye_width;
    int start_height = left_eye_height;
    int start_x = left_eye_x;
    int start_y = left_eye_y;

    // 목표 상태 설정
    int target_width = REF_EYE_WIDTH;
    int target_height = REF_EYE_HEIGHT;
    int target_x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
    int target_y = SCREEN_HEIGHT / 2;
    int target_droop = 0;

    switch (target_state)
    {
    case EYE_STATE_SAD:
        target_width = REF_EYE_WIDTH - 14;
        target_height = REF_EYE_HEIGHT - 20;
        target_droop = 15;
        break;
    case EYE_STATE_HAPPY:
    case EYE_STATE_SURPRISED:
        target_width = REF_EYE_WIDTH + 4;
        target_height = REF_EYE_HEIGHT + 4;
        break;
    default:
        // 일반 상태로 복귀
        break;
    }

    // 10단계로 부드럽게 전환
    for (int step = 0; step <= 10; step++)
    {
        float t = (float)step / 10.0f;

        apOledClear(0x00);

        // 보간된 값들
        int current_width = lerp(start_width, target_width, t);
        int current_height = lerp(start_height, target_height, t);
        int current_droop = lerp(0, target_droop, t);

        // 부드럽게 변하는 눈 그리기
        drawRoundRect(left_eye_x - current_width / 2,
                      left_eye_y - current_height / 2 + current_droop,
                      current_width, current_height, corner_radius, COLOR_WHITE);
        drawRoundRect(right_eye_x - current_width / 2,
                      right_eye_y - current_height / 2 + current_droop,
                      current_width, current_height, corner_radius, COLOR_WHITE);

        apOledUpdate();
    }

    current_eye_state = target_state;
}

void eyesInstantExpression(double wind_speed)
{
    static eye_state_t last_state = EYE_STATE_NORMAL;
    static uint32_t force_blink_timer = 0;
    uint32_t current_time = HAL_GetTick();

    eye_state_t new_state;

    printf("Wind: %.2f m/s\r\n", wind_speed); // 이제 정상 출력0;l
    // 범위 검사 (double로)
    if (wind_speed < 0.0 || wind_speed > 50.0 || isnan(wind_speed))
    {
        new_state = EYE_STATE_BLINK;
    }
    else
    {
        if (wind_speed > 5.0)
        {
            new_state = EYE_STATE_HAPPY;
        }
        else if (wind_speed > 2.0)
        {
            new_state = EYE_STATE_MOVE_RIGHT;
        }
        else if (wind_speed > 1.0)
        {
            new_state = EYE_STATE_MOVE_LEFT;
        }
        else
        {
            new_state = EYE_STATE_BLINK;
        }
    }

    // 나머지 로직은 동일...
    if (new_state != last_state)
    {
        switch (new_state)
        {
        case EYE_STATE_HAPPY:
            resetEyes(false);
            eyesHappy();
            break;
        case EYE_STATE_MOVE_LEFT:
            eyesMoveLeftBigEye();
            break;
        case EYE_STATE_MOVE_RIGHT:
            eyesMoveRightBigEye();
            break;
        case EYE_STATE_BLINK:
        default:
            eyesBlink(12);
            break;
        }
        last_state = new_state;
        force_blink_timer = current_time;
    }
    else if ((current_time - force_blink_timer) > 3000)
    {
        eyesBlink(12);
        force_blink_timer = current_time;
    }
}
