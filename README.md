
## EX_LED_BTN  
BTN1, BTN2 입력 상태를 LED로 확인하는 예제입니다.  

**설정 가이드**
- `ioc` 파일을 더블클릭하여 설정 화면으로 진입합니다.  
- **Project Manager → Code Generator** 메뉴에서  
  *Generated files 내 "Generate peripheral initialization as a pair of '.c/.h' files per peripheral"* 옵션을 체크합니다.  
- 해당 옵션을 설정하지 않으면 CubeIDE가 모든 초기화 코드를 `main.c`에 몰아서 생성합니다.  
- 파일 분할 관리가 용이하므로 본 옵션을 활성화하는 것을 권장합니다.  

---

## EX_OLED  
SPI 통신을 이용하여 OLED를 초기화하는 예제입니다.  

---

## EX_OLED_BTN_CTR  
OLED 초기화 후, BTN1/BTN2 입력 상태를 OLED에 표시하는 예제입니다.  

---

## EX_OLED_BTN_IT  
기존의 폴링 방식 대신 **인터럽트 방식**으로 버튼 입력을 처리하는 예제입니다.  
눌림 여부는 OLED로 확인합니다.  

**필요 설정**
- BTN1, BTN2 핀을 `GPIO_EXTI` 모드로 변경  
- Edge Trigger: Rising/Falling 선택  
- Pull-up 설정 적용  
- **System Core → NVIC** 메뉴에서 EXTI0, EXTI9_5 활성화  
- 저장 시 CubeIDE가 자동으로 인터럽트 핸들러 코드를 생성합니다.  

---

## EX_CAN_Loop
CAN 통신 기본 동작을 이해하고, UART를 통해 송수신 메시지를 확인하는 예제입니다.  

**CAN 특징**
- CAN Bus에 연결된 모든 노드는 특정 노드가 보낸 메시지를 모두 수신합니다.  
- 필요 메시지를 선별하기 위해 **필터링 설정**이 필요합니다.  

**설정 가이드**
- `Connectivity → FDCAN1` 활성화  
- NVIC 활성화  
- 주요 파라미터:  
  - Clock Divider: Divide kernel clock /2  
  - Frame Format: FD mode with BitRate Switching  
  - Mode: Internal Loopback  
  - Auto Retransmission: Disable  
  - Tx Fifo Queue: FIFO mode  
- Bit Timing Parameters (PLLCLK = 160 MHz → BaudRate = 500 kbps)  
  - Prescaler: 8  
  - Time Seg1: 13  
  - Time Seg2: 6  

---

## EX_CAN_Normal
외부 보드와 CAN 통신을 수행하는 예제입니다.  
- BTN 입력 시, STM32 HG TPM Controller 보드가 외부 보드로 CAN 메시지를 송신합니다.  
- PC의 CAN 프로그램에서 메시지를 전송하면 STM32 보드가 이를 수신합니다.  
- OLED 화면에는 송/수신된 메시지 개수를 카운트하여 표시합니다.  

---

## EX_SD_MOUNT  
SD 카드를 마운트하는 예제입니다.  
- 동작 결과는 시리얼 모니터(TeraTerm 등)로 확인할 수 있습니다.  

---

## EX_SD_IO  
SD 카드 마운트 후, 파일 입출력을 수행하는 예제입니다.  
- 파일 입출력은 C 표준 라이브러리 함수를 이용해 구현됩니다.  
- TeraTerm을 통해 사용자 입력을 받아 동작을 제어합니다.  

---

## EX_SD_OLED  
SD 카드 마운트 및 파일 입출력 여부를 확인하고, BMP 형식의 간단한 이미지를 OLED에 출력하는 예제입니다.  

---

## EX_MODBUS
Modbus RTU 를 사용하는 풍량센서 측정값을 실시간으로 받아보는 예제입니다.
 - TeraTerm을 통해 실시간으로 센서 측정값을 확인가능합니다.
 - 단순히 직접 timeout을 설정하고있고, DE핀도 제어하지 않고있어서 보통의 USART로 센서값을 받고있는것과 같습니다.

---

## EX_MODBUS_IDLE 
TC레지스터값을 확인하고, DE핀을 제어하여 반이중 통신으로서, 제활약을 하며, 
timeout을 직접 설정하지 않고, Idle line을 이용하여, Frame단위로 수신여부를 판단하여 안정적인 통신구조를 만든 예제입니다.

---

## EX_MODBUS_OLED
Modbus RTU 를 사용하는 풍량센서 측정값을 OLED에 출력하는 예제입니다.

---

자세한 설명은 아래 링크를 참조해 주세요.
# [STM32 HG TPM CONTROLLER](https://higenis.notion.site/STM32-HG-TPM-CONTROLLER-26379ff1b1b380b7944ec3346f1bc43b)