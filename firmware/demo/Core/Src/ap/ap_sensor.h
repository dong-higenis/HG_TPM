
#ifndef AP_SENSOR_H
#define AP_SENSOR_H

#include "modbus_def.h"

typedef struct {
    uint8_t  rx_buffer[MODBUS_RESPONSE_MAX];
    uint16_t rx_index;
    bool     frame_ready;
    uint32_t last_request_time;
    uint32_t request_interval;
    sensor_data_t data;
} wind_sensor_t;

bool apSensorInit(wind_sensor_t *sensor);
bool apSensorRequest(wind_sensor_t *sensor);
bool apSensorUpdate(wind_sensor_t *sensor);
float apSensorGetSpeed(wind_sensor_t *sensor);
bool apSensorIsValid(wind_sensor_t *sensor);

#endif
