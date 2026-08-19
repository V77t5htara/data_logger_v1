#ifndef SD_SPI_H
#define SD_SPI_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_SDSC,
    SD_TYPE_SDHC
} SD_Type;

HAL_StatusTypeDef SD_Init(void);
HAL_StatusTypeDef SD_ReadBlock(uint8_t *buf, uint32_t sector);
HAL_StatusTypeDef SD_WriteBlock(const uint8_t *buf, uint32_t sector);
uint32_t SD_GetSectorCount(void);
uint8_t SD_IsReady(void);

#endif
