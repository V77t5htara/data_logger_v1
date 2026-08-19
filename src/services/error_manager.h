#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

#include <stdint.h>

typedef enum {
    ERR_NONE   = 0,
    ERR_BME280 = 1U << 0,
    ERR_RTC    = 1U << 1,
    ERR_OLED   = 1U << 2,
    ERR_SD     = 1U << 3,
    ERR_FATFS  = 1U << 4,
    ERR_LOGGER = 1U << 5
} ErrorFlag;

void ErrorManager_Set(uint32_t flag);
void ErrorManager_Clear(uint32_t flag);
uint32_t ErrorManager_Get(void);
uint8_t ErrorManager_Has(uint32_t flag);

#endif
