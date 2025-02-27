/*
 * ssd1322_regs.h
 *
 *  Created on: 2024. 12. 27.
 *      Author: jangho
 */

#ifndef SRC_COMMON_HW_INCLUDE_LCD_SSD1322_REGS_H_
#define SRC_COMMON_HW_INCLUDE_LCD_SSD1322_REGS_H_

// 기본 색상 정의
#define SSD1322_BLACK                                 0 ///< 픽셀 끔
#define SSD1322_WHITE                                 1 ///< 픽셀 켬
#define SSD1322_INVERSE                               2 ///< 픽셀 반전

// 기본 명령어
#define SSD1322_ENABLE_GRAYSCALE_TABLE                0x00 ///< 그레이스케일 테이블 활성화
#define SSD1322_SET_COLUMN_ADDRESS                    0x15 ///< 열 주소 설정
#define SSD1322_WRITE_RAM                             0x5C ///< RAM 쓰기
#define SSD1322_READ_RAM                              0x5D ///< RAM 읽기
#define SSD1322_SET_ROW_ADDRESS                       0x75 ///< 행 주소 설정

// 디스플레이 설정 명령어
#define SSD1322_SET_REMAP_AND_DUAL_COM_LINE_MODE      0xA0 ///< 리맵 및 듀얼 COM 모드 설정
#define SSD1322_SET_DISPLAY_START_LINE                0xA1 ///< 디스플레이 시작 라인 설정
#define SSD1322_SET_DISPLAY_OFFSET                    0xA2 ///< 디스플레이 오프셋 설정
#define SSD1322_SET_DISPLAY_MODE_NORMAL               0xA6 ///< 일반 디스플레이 모드
#define SSD1322_SET_DISPLAY_MODE_INVERSE              0xA7 ///< 반전 디스플레이 모드
#define SSD1322_ENABLE_PARTIAL_DISPLAY                0xA8 ///< 부분 디스플레이 활성화
#define SSD1322_EXIT_PARTIAL_DISPLAY                  0xA9 ///< 부분 디스플레이 비활성화

// 전원 및 타이밍 설정 명령어
#define SSD1322_FUNCTION_SELECTION                    0xAB ///< 기능 선택 (내부/외부 전원)
#define SSD1322_DISPLAY_OFF                           0xAE ///< 디스플레이 끔
#define SSD1322_DISPLAY_ON                            0xAF ///< 디스플레이 켬
#define SSD1322_SET_PHASE_LENGTH                      0xB1 ///< 페이즈 길이 설정
#define SSD1322_SET_CLOCK_DIVIDER_AND_OSC_FREQ        0xB3 ///< 클럭 분주비 및 오실레이터 주파수 설정
#define SSD1322_SET_GPIO                              0xB5 ///< GPIO 핀 상태 설정
#define SSD1322_SET_SECOND_PRECHARGE_PERIOD           0xB6 ///< 두 번째 프리차지 기간 설정

// 그레이스케일 및 대비 설정 명령어
#define SSD1322_SET_GRAYSCALE_TABLE                   0xB8 ///< 그레이스케일 테이블 설정
#define SSD1322_SELECT_DEFAULT_LINEAR_GRAYSCALE_TABLE 0xB9 ///< 기본 선형 그레이스케일 테이블 선택
#define SSD1322_SET_PRECHARGE_VOLTAGE                 0xBB ///< 프리차지 전압 설정
#define SSD1322_SET_VCOMH_VOLTAGE                     0xBE ///< VCOMH 전압 설정
#define SSD1322_SET_CONTRAST_CURRENT                  0xC1 ///< 대비 전류 설정
#define SSD1322_MASTER_CONTRAST_CURRENT_CONTROL       0xC7 ///< 마스터 대비 전류 제어

// 다중화 비율 및 보호 명령어
#define SSD1322_SET_MULTIPLEX_RATIO                   0xCA ///< 다중화 비율 설정
#define SSD1322_DISPLAY_ENHANCEMENT_B                 0xD1 ///< 디스플레이 성능 향상 B

// 보호 및 잠금 명령어
#define SSD1322_SET_COMMAND_LOCK                      0xFD ///< 명령 잠금/해제

#define SSD1322_SETCOMMANDLOCK                        0xFD
#define SSD1322_DISPLAYOFF                            0xAE
#define SSD1322_DISPLAYON                             0xAF
#define SSD1322_SETCLOCKDIVIDER                       0xB3
#define SSD1322_SETDISPLAYOFFSET                      0xA2
#define SSD1322_SETSTARTLINE                          0xA1
#define SSD1322_SETREMAP                              0xA0
#define SSD1322_FUNCTIONSEL                           0xAB
#define SSD1322_DISPLAYENHANCE                        0xB4
#define SSD1322_SETCONTRASTCURRENT                    0xC1
#define SSD1322_MASTERCURRENTCONTROL                  0xC7
#define SSD1322_SETPHASELENGTH                        0xB1
#define SSD1322_DISPLAYENHANCEB                       0xD1
#define SSD1322_SETPRECHARGEVOLTAGE                   0xBB
#define SSD1322_SETSECONDPRECHARGEPERIOD              0xB6
#define SSD1322_SETVCOMH                              0xBE
#define SSD1322_NORMALDISPLAY                         0xA6
#define SSD1322_INVERSEDISPLAY                        0xA7
#define SSD1322_SETMUXRATIO                           0xCA
#define SSD1322_SETCOLUMNADDR                         0x15
#define SSD1322_SETROWADDR                            0x75
#define SSD1322_WRITERAM                              0x5C
#define SSD1322_ENTIREDISPLAYON                       0xA5
#define SSD1322_ENTIREDISPLAYOFF                      0xA4
#define SSD1322_SETGPIO                               0xB5
#define SSD1322_EXITPARTIALDISPLAY                    0xA9
#define SSD1322_SELECTDEFAULTGRAYSCALE                0xB9

#define MIN_SEG                                       0x1C
#define MAX_SEG                                       0x5B

// Scrolling #defines
#define SSD1322_ACTIVATE_SCROLL                       0x2F
#define SSD1322_DEACTIVATE_SCROLL                     0x2E
#define SSD1322_SET_VERTICAL_SCROLL_AREA              0xA3
#define SSD1322_RIGHT_HORIZONTAL_SCROLL               0x26
#define SSD1322_LEFT_HORIZONTAL_SCROLL                0x27
#define SSD1322_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL  0x29
#define SSD1322_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL   0x2A


#endif /* SRC_COMMON_HW_INCLUDE_LCD_SSD1322_REGS_H_ */
