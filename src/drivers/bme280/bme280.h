#ifndef BME280_H
#define BME280_H

#include "stm32f1xx_hal.h"

typedef struct {
    int32_t temperature_c_x100;
    uint32_t pressure_pa;
    uint32_t humidity_rh_x1000;
} BME280_Data;

HAL_StatusTypeDef BME280_Init(void);
HAL_StatusTypeDef BME280_ReadData(BME280_Data *data);

#endif
