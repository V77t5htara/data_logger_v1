#ifndef DS1302_H
#define DS1302_H

#include "stm32f1xx_hal.h"

typedef struct {
    uint16_t year;
    uint8_t month, day;
    uint8_t hour, minute, second;
} DS1302_DateTime;

HAL_StatusTypeDef DS1302_Init(void);
HAL_StatusTypeDef DS1302_GetDateTime(DS1302_DateTime *dt);
HAL_StatusTypeDef DS1302_SetDateTime(const DS1302_DateTime *dt);

#endif
