// Core/Src/hw/hw_modbus.h
#ifndef HW_MODBUS_H
#define HW_MODBUS_H

#include "modbus_def.h"


// CRC 계산
uint16_t hwModbusCRC16(uint8_t *data, uint16_t length);

// 요청 전송
bool hwModbusTransmit(uint8_t slave_addr, uint8_t func_code,
                      uint16_t start_addr, uint16_t reg_count);

// 응답 처리
bool hwModbusReceive(uint8_t *buffer, uint16_t length, sensor_data_t *sensor);

// RS485 DE 핀 제어
void hwModbusSetDE(bool enable);

#endif
