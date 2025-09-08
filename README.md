예제 1 ) EX_LED_BTN

    BTN1, 2의 누름여부를 LED로 확인합니다.

    ioc 파일 더블클릭으로 설정화면으로 진입합니다.
    Project Manager를 클릭, Code Generator 를 클릭합니다.
    Generated files 내 " Generate peripheral initialization as a pair of '.c/.h' files per peripheral "
    에 체크해줍니다. 
    해당 옵션을 설정하지 않을경우, CubeIDE (혹은 MX) 에서 자동으로
    생성해주는 코드가 main.c 파일에 몰리게 됩니다. 
    따라서 파일을 분할해주는 옵션이므로 키는것을 권장해 드립니다!

예제 2 ) EX_OLED
    
    OLED를 초기화하는 예제입니다.
    SPI 통신을 사용합니다.

예제 3 ) EX_OLED_BTN_CTR

    예제 2번의 OLED 초기화를 진행한 후, 예제 1번,
    버튼 눌림여부를 OLED에서 확인해볼수 있는 예제입니다.

예제 4 ) EX_OLED_BTN_IT
    - 기존 폴링방식으로 누름여부를 판단했던, 버튼을
    인터럽트 방식으로 바꾸는 예제입니다. 
    눌림여부는 OLED로 확인합니다.

    필요 설정 : 기존 BTN1, BTN2 핀을 GPIO_EXTI모드로 바꿔준 후
    Rising/Falling edge trigger 모드로 변경. pull-up 설정을 해줍니다.
    그후 System Core 메뉴 NVIC 진입, EXTI 0번대와 9:5를 찾고, 체크해줍니다.
    저장 하면 자동으로 인터럽트를 사용할수있게 CUBEIDE가 코드를 생성해줍니다.

예제 5 ) EX_CAN
    - CAN 통신을 이해하고, UART로 메시지 전송 , 수신을 확인해보는 예제입니다.

    CAN은 직렬통신방식중 하나입니다.
    CAN은 그 특성상, 
    CAN BUS에 접속되있는 모든 개체들은 한 개체가 전송한
    메시지를 다 들을수있습니다.
    따라서 필요한 메시지를 걸러내기 위해, 필터링을 거쳐주어야 합니다.  

    필요설정 :  ioc파일을 엽니다.
    Connectivity 메뉴의 FDCAN1 을 활성화 하고 핀은 그대로 사용합니다.
    FDCAN 설정 NVIC을 활성화 하고,
    Clock Devider : Divide kernel clock 2 
    Frame Format : FD mode with BitRate Switching 
    Mode : internal LoopBack mode
    Auto Retransmission : Disable
    Tx Fifo Queue Mode : FIFO mode 로하고,

    Bit Timing Parameters 설정값은

    Nominal Prescaler : 8
    Nominal Time Seg1 : 13
    Nominal Time Seg2 : 6

    PLLCLK 기준 160 [MHZ] 라면 500k [bit/s] 의 BaudRate를 확인 할 수 있습니다.
    
예제 6 ) EX_CAN_2_ESP
    
    외부 보드와 CAN 통신을 하는 예제입니다.
    버튼을 누르면 STM32 HG TPM CONTROLLER 보드에서 외부 보드로 CAN 메시지를 송신합니다.
    반대로 외부 보드와 연결된 PC상 CAN 프로그램에서 Send 버튼을 누르면,
    STM32 HG TPM CONTROLLER 보드에서 CAN 메시지가 수신됩니다.
    간단하게 OLED 화면으로 몇개의 메시지가 송/수신 되었는지 카운트 하고있는 모습입니다.

예제 7 ) EX_SD_MOUNT

    SD카드를 마운트 시키는 예제입니다.
    확인은 시리얼 모니터, 테라텀등으로 할 수 있습니다.

예제 8 ) EX_SD_IO

    SD카드를 마운트 시키고, 파일 입출력을 해보는 예제입니다.
    파일 입출력 관련 C함수로 구현을 하며, 테라텀으로 사용자의 입력을 받습니다.

예제 9 ) EX_SD_OLED

    SD카드 마운트 여부, 파일 입출력 여부등, 간단한 이미지 ( BMP 파일 )도 출력해보는 예제입니다.
