#ifndef BSP_H
#define BSP_H

#include "stm32f1xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

void SystemClock_Config(void);
void BSP_GPIO_Init(void);
void BSP_I2C1_Init(void);
void BSP_SPI1_Init(void);
void BSP_UART1_Init(void);

void BSP_UART_Write(const char *s);
void BSP_UART_WriteLine(const char *s);

#endif
