#include "hw_oled.h"

static bool oled_inited = false;

bool hwOledInit(void)
{
  bool ret = true;

  // SPI 초기화 확인
  if (hspi3.State != HAL_SPI_STATE_READY)
  {
    ret = false;
  }

  if (ret)
  {
    hwOledReset();

    // SSD1322 초기화 시퀀스
    hwOledSendCmd(OLED_DISPLAYOFF);

    hwOledSendCmd(OLED_SETCOMMANDLOCK);
    uint8_t temp_data = 0x12;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETCLOCKDIVIDER);
    temp_data = 0x91;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETMUXRATIO);
    temp_data = 0x3F;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETDISPLAYOFFSET);
    temp_data = 0x00; // 안 밀기
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETSTARTLINE);
    temp_data = 0x00;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETREMAP);
    uint8_t remap[2] = {0x06, 0x11}; // 반대로 뒤집기 {0x14, 0x11}
    hwOledSendData(remap, 2);

    hwOledSendCmd(OLED_SETGPIO);
    temp_data = 0x00;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_FUNCTIONSELECT);
    temp_data = 0x01;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_DISPLAYENHANCE_A);
    uint8_t enhA[2] = {0xA0, 0xFD};
    hwOledSendData(enhA, 2);

    hwOledSendCmd(OLED_SETCONTRAST);
    temp_data = 0xFF; // 최대 밝기
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_MASTERCURRENT);
    temp_data = 0x0F; // 최대
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_DEFAULTGRAYSCALE);

    hwOledSendCmd(OLED_SETPHASELENGTH);
    temp_data = 0xE2;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_DISPLAYENHANCE_B);
    uint8_t enhB[2] = {0x82, 0x20};
    hwOledSendData(enhB, 2);

    hwOledSendCmd(OLED_SETPRECHARGEVOLTAGE);
    temp_data = 0x1F;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETSECONDPRECHARGE);
    temp_data = 0x08;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_SETVCOMH);
    temp_data = 0x07;
    hwOledSendData(&temp_data, 1);

    hwOledSendCmd(OLED_NORMALDISPLAY);

    hwOledSendCmd(OLED_EXITPARTIALDISPLAY);

    hwOledSetWindow(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

    hwOledSendCmd(OLED_DISPLAYON);
    HAL_Delay(50);

    oled_inited = true;
  }

  return ret;
}

void hwOledReset(void)
{
  OLED_RST_L();
  HAL_Delay(10);
  OLED_RST_H();
  HAL_Delay(10);
}

bool hwOledSendCmd(uint8_t cmd)
{
  bool ret = true;

  OLED_CS_L();
  OLED_DC_CMD();

  if (HAL_SPI_Transmit(&hspi3, &cmd, 1, 100) != HAL_OK)
  {
    ret = false;
  }

  OLED_CS_H();

  return ret;
}

bool hwOledSendData(uint8_t *p_data, uint32_t length)
{
  bool ret = true;

  OLED_CS_L();
  OLED_DC_DATA();

  if (HAL_SPI_Transmit(&hspi3, p_data, length, 1000) != HAL_OK)
  {
    ret = false;
  }

  OLED_CS_H();

  return ret;
}

void hwOledSetWindow(uint8_t col_start, uint8_t col_end, uint8_t row_start, uint8_t row_end)
{
  // Set Column Address
  hwOledSendCmd(OLED_SETCOLUMNADDR);
  uint8_t col_data[2] = {col_start, col_end};
  hwOledSendData(col_data, 2);

  // Set Row Address
  hwOledSendCmd(OLED_SETROWADDR);
  uint8_t row_data[2] = {row_start, row_end};
  hwOledSendData(row_data, 2);

  // Write RAM
  hwOledSendCmd(OLED_WRITERAM);
}
