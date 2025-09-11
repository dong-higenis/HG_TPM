
#include "hw_modbus.h"
#include "main.h"
#include <stdio.h>

uint16_t hwModbusCRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

bool hwModbusTransmit(uint8_t slave_addr, uint8_t func_code,
                      uint16_t start_addr, uint16_t reg_count)
{
    uint8_t request[MODBUS_REQUEST_SIZE];

    // 프레임 구성
    request[0] = slave_addr;
    request[1] = func_code;
    request[2] = (start_addr >> 8) & 0xFF;
    request[3] = start_addr & 0xFF;
    request[4] = (reg_count >> 8) & 0xFF;
    request[5] = reg_count & 0xFF;

    // CRC 계산
    uint16_t crc = hwModbusCRC16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    // RS485 송신 모드
    hwModbusSetDE(true);

    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, request,
                                                 MODBUS_REQUEST_SIZE, 1000);

    // 전송 완료 대기
    while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET)
        ;

    // RS485 수신 모드
    hwModbusSetDE(false);

    return (status == HAL_OK);
}

bool hwModbusReceive(uint8_t *buffer, uint16_t length, sensor_data_t *sensor)
{
    if (length < 5)
        return false;

    // CRC 검증
    uint16_t calc_crc = hwModbusCRC16(buffer, length - 2);
    uint16_t recv_crc = buffer[length - 2] | (buffer[length - 1] << 8);

    if (calc_crc != recv_crc)
        return false;

    // 응답 파싱
    modbus_response_t *response = (modbus_response_t *)buffer;

    if (response->slave_addr == MODBUS_SLAVE_ADDR &&
        response->function_code == MODBUS_FUNC_READ_HOLD &&
        response->byte_count >= 2)
    {

        uint16_t raw_data = (response->data[0] << 8) | response->data[1];
        sensor->wind_speed = raw_data * 0.1f;
        sensor->timestamp = HAL_GetTick();
        sensor->valid = true;

        return true;
    }

    return false;
}

void hwModbusSetDE(bool enable)
{
    HAL_GPIO_WritePin(Modbus_DE_GPIO_Port, Modbus_DE_Pin,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
