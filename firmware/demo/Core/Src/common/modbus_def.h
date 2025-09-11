// Core/Src/common/modbus_def.h
#ifndef MODBUS_DEF_H
#define MODBUS_DEF_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

// Modbus 상수 정의
#define MODBUS_SLAVE_ADDR 0x01
#define MODBUS_FUNC_READ_HOLD 0x03
#define MODBUS_REQUEST_SIZE 8
#define MODBUS_RESPONSE_MAX 256

// 풍량센서 레지스터
#define WIND_SPEED_REG 0x0000

extern UART_HandleTypeDef huart3;

// Modbus 프레임 구조체
typedef struct
{
    uint8_t slave_addr;
    uint8_t function_code;
    uint16_t start_addr;
    uint16_t register_count;
    uint16_t crc;
} __attribute__((packed)) modbus_request_t;

typedef struct
{
    uint8_t slave_addr;
    uint8_t function_code;
    uint8_t byte_count;
    uint8_t data[64];
    uint16_t crc;
} __attribute__((packed)) modbus_response_t;

// 센서 데이터 구조체
typedef struct
{
    float wind_speed;
    uint32_t timestamp;
    bool valid;
} sensor_data_t;

#endif
