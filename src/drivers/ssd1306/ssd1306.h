#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f1xx_hal.h"

HAL_StatusTypeDef SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_DrawString(uint8_t x, uint8_t y, const char *s);
HAL_StatusTypeDef SSD1306_Update(void);

#endif
