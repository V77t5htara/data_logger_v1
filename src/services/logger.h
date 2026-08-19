#ifndef LOGGER_H
#define LOGGER_H

#include "stm32f1xx_hal.h"
#include "drivers/bme280/bme280.h"
#include "drivers/ds1302/ds1302.h"

HAL_StatusTypeDef Logger_Init(void);
HAL_StatusTypeDef Logger_Write(const DS1302_DateTime *dt,
                               const BME280_Data *data,
                               uint32_t sequence);
void Logger_Close(void);

#endif
