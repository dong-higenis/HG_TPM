// Core/Src/ap/ap_sensor.c
#include "ap_sensor.h"
#include "main.h"
#include <string.h>

// Modbus 상수
#define MODBUS_SLAVE_ADDR      0x01
#define MODBUS_FUNC_READ_HOLD  0x03
#define WIND_SPEED_REG         0x0000

// CRC16 계산 함수
static uint16_t calcCRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

bool apSensorInit(wind_sensor_t *sensor)
{
    memset(sensor, 0, sizeof(wind_sensor_t));
    sensor->data.wind_speed = 0.0f;
    sensor->request_interval = 100; // 100ms

    // UART3 DMA 수신 시작
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sensor->rx_buffer,
                                 sizeof(sensor->rx_buffer));

    printf("Sensor Init Complete\r\n");
    return 1;
}

bool apSensorRequest(wind_sensor_t *sensor)
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - sensor->last_request_time >= sensor->request_interval) {
        uint8_t request[8];

        // Modbus 요청 프레임 구성
        request[0] = MODBUS_SLAVE_ADDR;
        request[1] = MODBUS_FUNC_READ_HOLD;
        request[2] = (WIND_SPEED_REG >> 8) & 0xFF;
        request[3] = WIND_SPEED_REG & 0xFF;
        request[4] = 0x00; // 레지스터 개수 상위바이트
        request[5] = 0x02; // 레지스터 개수 하위바이트 (2개)

        // CRC 계산
        uint16_t crc = calcCRC16(request, 6);
        request[6] = crc & 0xFF;
        request[7] = (crc >> 8) & 0xFF;

        // RS485 송신 모드
        HAL_GPIO_WritePin(Modbus_DE_GPIO_Port, Modbus_DE_Pin, GPIO_PIN_SET);

        // UART3로 전송
        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, request, 8, 1000);

        // 전송 완료 대기
        while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET);

        // RS485 수신 모드
        HAL_GPIO_WritePin(Modbus_DE_GPIO_Port, Modbus_DE_Pin, GPIO_PIN_RESET);

        sensor->last_request_time = current_time;

        if (status == HAL_OK) {
            return 1;
        } else {
            printf("Modbus Request Failed: %d\r\n", status);
        }
    }

    return 0;
}

bool apSensorUpdate(wind_sensor_t *sensor)
{
    if (sensor->frame_ready && sensor->rx_index >= 7) {

        // CRC 검증
        uint16_t calc_crc = calcCRC16(sensor->rx_buffer, sensor->rx_index - 2);
        uint16_t recv_crc = sensor->rx_buffer[sensor->rx_index - 2] |
                           (sensor->rx_buffer[sensor->rx_index - 1] << 8);

        if (calc_crc == recv_crc) {
            // 응답 데이터 파싱
            if (sensor->rx_buffer[0] == MODBUS_SLAVE_ADDR &&
                sensor->rx_buffer[1] == MODBUS_FUNC_READ_HOLD &&
                sensor->rx_buffer[2] >= 4) {

                uint16_t raw_data = (sensor->rx_buffer[3] << 8) | sensor->rx_buffer[4];
                sensor->data.wind_speed = raw_data * 0.1f;
                sensor->data.timestamp = HAL_GetTick();
                sensor->data.valid = 1;

            }
        } else {
            printf("CRC Error: calc=0x%04X, recv=0x%04X\r\n", calc_crc, recv_crc);
        }

        // 프레임 처리 완료
        sensor->rx_index = 0;
        sensor->frame_ready = 0;
        memset(sensor->rx_buffer, 0, sizeof(sensor->rx_buffer));

        return 1;
    }

    return 0;
}

float apSensorGetSpeed(wind_sensor_t *sensor)
{
    return sensor->data.wind_speed;
}

bool apSensorIsValid(wind_sensor_t *sensor)
{
    return sensor->data.valid;
}
