#include "oled_eyes.h"

// 전역 상태 변수들
int ref_eye_height = REF_EYE_HEIGHT;
int ref_eye_width = REF_EYE_WIDTH;
int ref_space_between_eye = REF_SPACE_BETWEEN_EYE;
int ref_corner_radius = REF_CORNER_RADIUS;

// 현재 눈 상태
int left_eye_height = REF_EYE_HEIGHT;
int left_eye_width = REF_EYE_WIDTH;
int left_eye_x = 78;   // SCREEN_WIDTH/2 - REF_EYE_WIDTH/2 - REF_SPACE_BETWEEN_EYE/2
int left_eye_y = 32;   // SCREEN_HEIGHT/2
int right_eye_x = 178; // SCREEN_WIDTH/2 + REF_EYE_WIDTH/2 + REF_SPACE_BETWEEN_EYE/2
int right_eye_y = 32;
int right_eye_height = REF_EYE_HEIGHT;
int right_eye_width = REF_EYE_WIDTH;
int corner_radius = REF_CORNER_RADIUS;

void eyesInit(void)
{
    resetEyes(true);
}

void drawRoundRect(int x, int y, int w, int h, int r, uint8_t color)
{
    if (r <= 0)
    {
        // 반지름이 0이면 그냥 사각형
        apOledDrawFilledRect(x, y, w, h, color);
        return;
    }

    // 반지름이 너무 크면 조정
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;

    // 1. 중앙 사각형들 그리기
    apOledDrawFilledRect(x + r, y, w - 2 * r, h, color);         // 가로 중앙
    apOledDrawFilledRect(x, y + r, r, h - 2 * r, color);         // 좌측 세로
    apOledDrawFilledRect(x + w - r, y + r, r, h - 2 * r, color); // 우측 세로

    // 2. 네 모서리에 1/4 원 그리기
    drawQuarterCircle(x + r, y + r, r, 2, color);                 // 좌상단 (2사분면)
    drawQuarterCircle(x + w - r - 1, y + r, r, 1, color);         // 우상단 (1사분면)
    drawQuarterCircle(x + r, y + h - r - 1, r, 3, color);         // 좌하단 (3사분면)
    drawQuarterCircle(x + w - r - 1, y + h - r - 1, r, 4, color); // 우하단 (4사분면)
}

// 1/4 원 그리기 함수
void drawQuarterCircle(int cx, int cy, int radius, int quadrant, uint8_t color)
{
    for (int y = 0; y <= radius; y++)
    {
        for (int x = 0; x <= radius; x++)
        {
            // 원의 방정식: x² + y² ≤ r²
            if (x * x + y * y <= radius * radius)
            {
                int px, py;

                switch (quadrant)
                {
                case 1: // 1사분면 (오른쪽 위)
                    px = cx + x;
                    py = cy - y;
                    break;
                case 2: // 2사분면 (왼쪽 위)
                    px = cx - x;
                    py = cy - y;
                    break;
                case 3: // 3사분면 (왼쪽 아래)
                    px = cx - x;
                    py = cy + y;
                    break;
                case 4: // 4사분면 (오른쪽 아래)
                    px = cx + x;
                    py = cy + y;
                    break;
                default:
                    continue;
                }

                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                {
                    apOledDrawPixel(px, py, color);
                }
            }
        }
    }
}

void drawEyes(bool update)
{
    apOledClear(0x00);

    // 좌측 눈 (중심 좌표에서 그리기)
    int x = left_eye_x - left_eye_width / 2;
    int y = left_eye_y - left_eye_height / 2;

    // 경계 체크
    if (left_eye_width > 0 && left_eye_height > 0)
    {
        drawRoundRect(x, y, left_eye_width, left_eye_height, corner_radius, COLOR_WHITE);
    }

    // 우측 눈
    x = right_eye_x - right_eye_width / 2;
    y = right_eye_y - right_eye_height / 2;

    if (right_eye_width > 0 && right_eye_height > 0)
    {
        drawRoundRect(x, y, right_eye_width, right_eye_height, corner_radius, COLOR_WHITE);
    }

    if (update)
    {
        apOledUpdate();
    }
}

void resetEyes(bool update)
{
    // 눈을 화면 중앙으로 이동
    left_eye_height = ref_eye_height;
    left_eye_width = ref_eye_width;
    right_eye_height = ref_eye_height;
    right_eye_width = ref_eye_width;

    left_eye_x = SCREEN_WIDTH / 2 - ref_eye_width / 2 - ref_space_between_eye / 2;
    left_eye_y = SCREEN_HEIGHT / 2;
    right_eye_x = SCREEN_WIDTH / 2 + ref_eye_width / 2 + ref_space_between_eye / 2;
    right_eye_y = SCREEN_HEIGHT / 2;

    corner_radius = ref_corner_radius;
    drawEyes(update);
}

void eyesBlink(int speed)
{
    resetEyes(false);
    drawEyes(true);

    // 눈 감기
    for (int i = 0; i < 3; i++)
    {
        left_eye_height = left_eye_height - speed;
        right_eye_height = right_eye_height - speed;
        left_eye_width = left_eye_width + 3;
        right_eye_width = right_eye_width + 3;
        drawEyes(true);
    }

    // 눈 뜨기
    for (int i = 0; i < 3; i++)
    {
        left_eye_height = left_eye_height + speed;
        right_eye_height = right_eye_height + speed;
        left_eye_width = left_eye_width - 3;
        right_eye_width = right_eye_width - 3;
        drawEyes(true);
    }
}

void eyesSleep(void)
{
    resetEyes(false);
    left_eye_height = 4; // 원본 2 → 4 (2배)
    left_eye_width = ref_eye_width;
    right_eye_height = 4;
    right_eye_width = ref_eye_width;
    corner_radius = 0;
    drawEyes(true);
}

void eyesWakeup(void)
{
    resetEyes(false);
    eyesSleep();

    for (int h = 4; h <= ref_eye_height; h += 4)
    { // 원본 2 → 4 (2배)
        left_eye_height = h;
        right_eye_height = h;
        corner_radius = h / 2;
        if (corner_radius > ref_corner_radius)
        {
            corner_radius = ref_corner_radius;
        }
        drawEyes(true);
    }
}

void eyesHappy(void)
{
    apOledClear(0x00);

    int arch_height = 15;

    // 좌측 눈 - 아래쪽 볼록 아치 (∪ 모양)
    for (int x = 0; x < left_eye_width; x++)
    {
        float normalized_x = (float)(x - left_eye_width / 2) / (left_eye_width / 2);
        int y_offset = (int)(arch_height * (normalized_x * normalized_x)); // 역방향!

        int pixel_x = left_eye_x - left_eye_width / 2 + x;
        int start_y = left_eye_y - arch_height / 2 + y_offset; // 아래에서 시작
        int end_y = left_eye_y + arch_height / 2;              // 위까지

        for (int y = start_y; y <= end_y; y++)
        {
            if (pixel_x >= 0 && pixel_x < SCREEN_WIDTH &&
                y >= 0 && y < SCREEN_HEIGHT)
            {
                apOledDrawPixel(pixel_x, y, COLOR_WHITE);
            }
        }
    }

    // 우측 눈도 동일하게
    for (int x = 0; x < right_eye_width; x++)
    {
        float normalized_x = (float)(x - right_eye_width / 2) / (right_eye_width / 2);
        int y_offset = (int)(arch_height * (normalized_x * normalized_x));

        int pixel_x = right_eye_x - right_eye_width / 2 + x;
        int start_y = right_eye_y - arch_height / 2 + y_offset;
        int end_y = right_eye_y + arch_height / 2;

        for (int y = start_y; y <= end_y; y++)
        {
            if (pixel_x >= 0 && pixel_x < SCREEN_WIDTH &&
                y >= 0 && y < SCREEN_HEIGHT)
            {
                apOledDrawPixel(pixel_x, y, COLOR_WHITE);
            }
        }
    }

    apOledUpdate();
    HAL_Delay(500);
}

void eyesSaccade(int direction_x, int direction_y)
{
    int direction_x_movement_amplitude = 15; // 원본 8 → 15 (256x64용)
    int direction_y_movement_amplitude = 12; // 원본 6 → 12
    int blink_amplitude = 15;                // 원본 8 → 15

    for (int i = 0; i < 1; i++)
    {
        left_eye_x += direction_x_movement_amplitude * direction_x;
        right_eye_x += direction_x_movement_amplitude * direction_x;
        left_eye_y += direction_y_movement_amplitude * direction_y;
        right_eye_y += direction_y_movement_amplitude * direction_y;

        right_eye_height -= blink_amplitude;
        left_eye_height -= blink_amplitude;
        drawEyes(true);
        HAL_Delay(50);
    }

    for (int i = 0; i < 1; i++)
    {
        left_eye_x += direction_x_movement_amplitude * direction_x;
        right_eye_x += direction_x_movement_amplitude * direction_x;
        left_eye_y += direction_y_movement_amplitude * direction_y;
        right_eye_y += direction_y_movement_amplitude * direction_y;

        right_eye_height += blink_amplitude;
        left_eye_height += blink_amplitude;
        drawEyes(true);
        HAL_Delay(50);
    }
}

void eyesMoveBigEye(int direction)
{
    // direction == -1: move left, direction == 1: move right
    int direction_oversize = 2;           // 원본 1 → 2 (256x64용)
    int direction_movement_amplitude = 4; // 원본 2 → 4
    int blink_amplitude = 10;             // 원본 5 → 10

    // 1단계: 이동 + 깜빡임 + 한쪽 눈 커지기
    for (int i = 0; i < 3; i++)
    {
        left_eye_x += direction_movement_amplitude * direction;
        right_eye_x += direction_movement_amplitude * direction;
        right_eye_height -= blink_amplitude;
        left_eye_height -= blink_amplitude;

        if (direction > 0)
        { // 오른쪽으로 이동시 우측 눈 커지기
            right_eye_height += direction_oversize;
            right_eye_width += direction_oversize;
        }
        else
        { // 왼쪽으로 이동시 좌측 눈 커지기
            left_eye_height += direction_oversize;
            left_eye_width += direction_oversize;
        }
        drawEyes(true);
    }

    // 2단계: 계속 이동 + 눈 뜨기 + 계속 커지기
    for (int i = 0; i < 3; i++)
    {
        left_eye_x += direction_movement_amplitude * direction;
        right_eye_x += direction_movement_amplitude * direction;
        right_eye_height += blink_amplitude;
        left_eye_height += blink_amplitude;

        if (direction > 0)
        {
            right_eye_height += direction_oversize;
            right_eye_width += direction_oversize;
        }
        else
        {
            left_eye_height += direction_oversize;
            left_eye_width += direction_oversize;
        }
        drawEyes(true);
    }

    HAL_Delay(1000); // 1초 대기

    // 3단계: 원래 위치로 돌아가기 + 깜빡임 + 눈 작아지기
    for (int i = 0; i < 3; i++)
    {
        left_eye_x -= direction_movement_amplitude * direction;
        right_eye_x -= direction_movement_amplitude * direction;
        right_eye_height -= blink_amplitude;
        left_eye_height -= blink_amplitude;

        if (direction > 0)
        {
            right_eye_height -= direction_oversize;
            right_eye_width -= direction_oversize;
        }
        else
        {
            left_eye_height -= direction_oversize;
            left_eye_width -= direction_oversize;
        }
        drawEyes(true);
    }

    // 4단계: 완전히 원래대로
    for (int i = 0; i < 3; i++)
    {
        left_eye_x -= direction_movement_amplitude * direction;
        right_eye_x -= direction_movement_amplitude * direction;
        right_eye_height += blink_amplitude;
        left_eye_height += blink_amplitude;

        if (direction > 0)
        {
            right_eye_height -= direction_oversize;
            right_eye_width -= direction_oversize;
        }
        else
        {
            left_eye_height -= direction_oversize;
            left_eye_width -= direction_oversize;
        }
        drawEyes(true);
    }

    resetEyes(true); // 완전히 원래 상태로 복원
}

void eyesMoveRightBigEye(void)
{
    eyesMoveBigEye(1);
}

void eyesMoveLeftBigEye(void)
{
    eyesMoveBigEye(-1);
}

void eyesSurprisedAnimation(void)
{
    // 1단계: 일반 눈에서 시작
    resetEyes(false);
    drawEyes(true);
    HAL_Delay(100);

    // 2단계: 아주 살짝만 커지기 (미세한 변화!)
    for (int i = 0; i < 3; i++)
    {
        apOledClear(0x00);

        // 기존 ref_eye_width=60, ref_eye_height=50
        // 최대 6픽셀만 증가하게 변경!
        int growth = i * 8; // 0, 2, 4픽셀만 증가
        int current_width = ref_eye_width + growth;
        int current_height = ref_eye_height + growth;

        // 원형 대신 둥근 사각형으로 (더 자연스럽고 작음)
        drawRoundRect(left_eye_x - current_width / 2, left_eye_y - current_height / 2,
                      current_width, current_height, corner_radius + i, COLOR_WHITE);
        drawRoundRect(right_eye_x - current_width / 2, right_eye_y - current_height / 2,
                      current_width, current_height, corner_radius + i, COLOR_WHITE);

        apOledUpdate();
        HAL_Delay(20);
    }

    // 3단계: 아주 살짝만 떨기 (미묘한 움직임)
    for (int i = 0; i < 6; i++)
    {
        int shake_x = (i % 2 == 0) ? 1 : -1; // 1픽셀씩만 흔들기

        apOledClear(0x00);

        // 놀란 상태 크기: 기존 + 4픽셀만
        int surprised_width = ref_eye_width + 12;
        int surprised_height = ref_eye_height + 12;

        // 미세하게 흔들리는 눈
        drawRoundRect(left_eye_x - surprised_width / 2 + shake_x,
                      left_eye_y - surprised_height / 2,
                      surprised_width, surprised_height, corner_radius, COLOR_WHITE);
        drawRoundRect(right_eye_x - surprised_width / 2 + shake_x,
                      right_eye_y - surprised_height / 2,
                      surprised_width, surprised_height, corner_radius, COLOR_WHITE);

        apOledUpdate();
        HAL_Delay(20);
    }

    // 4단계: 빠르게 원래대로
    for (int i = 2; i >= 0; i--)
    {
        apOledClear(0x00);

        int shrink = i * 2; // 4, 2, 0픽셀 감소
        int current_width = ref_eye_width + shrink;
        int current_height = ref_eye_height + shrink;

        drawRoundRect(left_eye_x - current_width / 2, left_eye_y - current_height / 2,
                      current_width, current_height, corner_radius, COLOR_WHITE);
        drawRoundRect(right_eye_x - current_width / 2, right_eye_y - current_height / 2,
                      current_width, current_height, corner_radius, COLOR_WHITE);

        apOledUpdate();
        HAL_Delay(20);
    }

    // 원래 상태로 복귀
    resetEyes(true);
}

void eyesSadAnimation(void)
{
    // 1단계: 일반 눈에서 점점 슬퍼지기
    for (int sad_level = 0; sad_level < 8; sad_level++)
    {
        apOledClear(0x00);

        // 점점 작아지고 아래로 처지는 눈
        int current_width = ref_eye_width - sad_level * 2;   // 점점 작아짐
        int current_height = ref_eye_height - sad_level * 3; // 더 많이 작아짐
        int droop_offset = sad_level * 2;                    // 아래로 처짐

        // 좌측 슬픈 눈 (작고 아래로)
        drawRoundRect(left_eye_x - current_width / 2,
                      left_eye_y - current_height / 2 + droop_offset,
                      current_width, current_height, corner_radius / 2, COLOR_WHITE);

        // 우측 슬픈 눈 (작고 아래로)
        drawRoundRect(right_eye_x - current_width / 2,
                      right_eye_y - current_height / 2 + droop_offset,
                      current_width, current_height, corner_radius / 2, COLOR_WHITE);

        apOledUpdate();
        HAL_Delay(10);
    }

    HAL_Delay(30);

    // 2단계: 슬픈 눈에서 엄청 큰 눈물 떨어지기
    for (int tear_drop = 0; tear_drop < 25; tear_drop++)
    {
        apOledClear(0x00);
        // 최종 슬픈 눈 (아주 작고 아래로 처진)
        int sad_width = ref_eye_width - 14;
        int sad_height = ref_eye_height - 20;
        int final_droop = 15;

        drawRoundRect(left_eye_x - sad_width / 2,
                      left_eye_y - sad_height / 2 + final_droop,
                      sad_width, sad_height, 2, COLOR_WHITE);
        drawRoundRect(right_eye_x - sad_width / 2,
                      right_eye_y - sad_height / 2 + final_droop,
                      sad_width, sad_height, 2, COLOR_WHITE);

        // 좌측 엄청 큰 눈물 (5x5 크기!)
        int tear_y = left_eye_y + final_droop + sad_height / 2 + 3 + tear_drop * 2;
        if (tear_y + 4 < SCREEN_HEIGHT)
        {
            // 큰 눈물 방울 (5x5)
            apOledDrawFilledRect(left_eye_x - 2, tear_y, 5, 5, 0x0F); // 메인 방울
            apOledDrawPixel(left_eye_x - 2, tear_y - 1, 0x0A);        // 위쪽 테두리
            apOledDrawPixel(left_eye_x + 2, tear_y - 1, 0x0A);
            apOledDrawPixel(left_eye_x, tear_y - 1, 0x0C); // 위쪽 중앙

            // 긴 눈물 흔적 (수직으로 길게)
            for (int trail = 1; trail <= 8 && tear_drop > trail; trail++)
            {
                int trail_y = tear_y - trail * 2;
                uint8_t trail_brightness = 0x0C - (trail * 1);
                if (trail_brightness > 0 && trail_y >= 0)
                {
                    apOledDrawPixel(left_eye_x, trail_y, trail_brightness);
                    if (trail < 4)
                    { // 처음 몇 개는 두껍게
                        apOledDrawPixel(left_eye_x - 1, trail_y, trail_brightness - 2);
                        apOledDrawPixel(left_eye_x + 1, trail_y, trail_brightness - 2);
                    }
                }
            }
        }

        // 우측 엄청 큰 눈물 (5x5 크기!)
        tear_y = right_eye_y + final_droop + sad_height / 2 + 3 + tear_drop * 2;
        if (tear_y + 4 < SCREEN_HEIGHT)
        {
            // 큰 눈물 방울 (5x5)
            apOledDrawFilledRect(right_eye_x - 2, tear_y, 5, 5, 0x0F); // 메인 방울
            apOledDrawPixel(right_eye_x - 2, tear_y - 1, 0x0A);        // 위쪽 테두리
            apOledDrawPixel(right_eye_x + 2, tear_y - 1, 0x0A);
            apOledDrawPixel(right_eye_x, tear_y - 1, 0x0C); // 위쪽 중앙

            // 긴 눈물 흔적
            for (int trail = 1; trail <= 8 && tear_drop > trail; trail++)
            {
                int trail_y = tear_y - trail * 2;
                uint8_t trail_brightness = 0x0C - (trail * 1);
                if (trail_brightness > 0 && trail_y >= 0)
                {
                    apOledDrawPixel(right_eye_x, trail_y, trail_brightness);
                    if (trail < 4)
                    {
                        apOledDrawPixel(right_eye_x - 1, trail_y, trail_brightness - 2);
                        apOledDrawPixel(right_eye_x + 1, trail_y, trail_brightness - 2);
                    }
                }
            }
        }
        apOledUpdate();
    }

    HAL_Delay(20); // 슬픈 표정 유지

    // 3단계: 점점 원래 눈으로 돌아가기 (역방향 애니메이션)
    for (int recover_level = 7; recover_level >= 0; recover_level--)
    {
        apOledClear(0x00);

        // 점점 커지고 원래 위치로 올라가는 눈
        int current_width = ref_eye_width - recover_level * 2;   // 점점 커짐
        int current_height = ref_eye_height - recover_level * 3; // 점점 커짐
        int droop_offset = recover_level * 2;                    // 원래 위치로

        // 좌측 눈 (점점 원래대로)
        drawRoundRect(left_eye_x - current_width / 2,
                      left_eye_y - current_height / 2 + droop_offset,
                      current_width, current_height, corner_radius / 2 + recover_level, COLOR_WHITE);

        // 우측 눈 (점점 원래대로)
        drawRoundRect(right_eye_x - current_width / 2,
                      right_eye_y - current_height / 2 + droop_offset,
                      current_width, current_height, corner_radius / 2 + recover_level, COLOR_WHITE);

        apOledUpdate();
        HAL_Delay(20); // 회복은 조금 더 천천히
    }

    // 4단계: 완전히 원래 눈으로 복귀
    resetEyes(true);
    HAL_Delay(50);
}

void eyesLaunchAnimation(int animation_index)
{
    if (animation_index > 8)
    {
        animation_index = 8;
    }

    switch (animation_index)
    {
    case 0:
        eyesWakeup();
        break;
    case 1:
        resetEyes(true);
        break;
    case 2:
        eyesMoveRightBigEye();
        break;
    case 3:
        eyesMoveLeftBigEye();
        break;
    case 4:
        eyesBlink(12);
        HAL_Delay(1000);
        break;
    case 5:
        eyesBlink(12);
        break;
    case 6:
        eyesHappy();
        break;
    case 7:
        eyesSleep();
        break;
    case 8:
        resetEyes(true);
        for (int i = 0; i < 20; i++)
        {
            int dir_x = (rand() % 3) - 1; // -1, 0, 1
            int dir_y = (rand() % 3) - 1;
            eyesSaccade(dir_x, dir_y);
            HAL_Delay(100);
            eyesSaccade(-dir_x, -dir_y);
            HAL_Delay(100);
        }
        break;
    case 9:
        eyesSurprisedAnimation();
        break; // 새로 추가!
    case 10:
        eyesSadAnimation();
        break; // 새로 추가!
    default:
        resetEyes(true);
        break;
    }
}
