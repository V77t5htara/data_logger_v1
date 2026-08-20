#include "ds1302.h"
#include "bsp/bsp.h"
#include "app_config.h"
#include <stdio.h>

#define CMD_SEC_R 0x81
#define CMD_MIN_R 0x83
#define CMD_HOUR_R 0x85
#define CMD_DATE_R 0x87
#define CMD_MONTH_R 0x89
#define CMD_DAY_R 0x8B
#define CMD_YEAR_R 0x8D
#define CMD_CTRL_R 0x8F

#define CMD_SEC_W 0x80
#define CMD_MIN_W 0x82
#define CMD_HOUR_W 0x84
#define CMD_DATE_W 0x86
#define CMD_MONTH_W 0x88
#define CMD_DAY_W 0x8A
#define CMD_YEAR_W 0x8C
#define CMD_CTRL_W 0x8E

static uint8_t bcd2bin(uint8_t x) { return (uint8_t)((x >> 4) * 10U + (x & 0x0F)); }
static uint8_t bin2bcd(uint8_t x) { return (uint8_t)(((x / 10U) << 4) | (x % 10U)); }

static void io_output(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DS1302_IO_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_IO_PORT, &g);
}

static void io_input(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DS1302_IO_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS1302_IO_PORT, &g);
}

static void clk(int level)
{
    HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    for (volatile int i = 0; i < 8; ++i) {}
}

static void write_byte(uint8_t v)
{
    io_output();
    for (uint8_t i = 0; i < 8; ++i) {
        HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN,
                          (v & (1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        clk(1);
        clk(0);
    }
}

static uint8_t read_byte(void)
{
    uint8_t v = 0;

    io_input();

    for (uint8_t i = 0; i < 8; ++i) {
        clk(1);
        clk(0);

        if (HAL_GPIO_ReadPin(DS1302_IO_PORT, DS1302_IO_PIN) == GPIO_PIN_SET)
            v |= (uint8_t)(1U << i);
    }

    return v;
}

static uint8_t read_reg(uint8_t cmd)
{
    HAL_GPIO_WritePin(DS1302_CE_PORT, DS1302_CE_PIN, GPIO_PIN_SET);
    write_byte(cmd);
    uint8_t v = read_byte();
    HAL_GPIO_WritePin(DS1302_CE_PORT, DS1302_CE_PIN, GPIO_PIN_RESET);
    return v;
}

static void write_reg(uint8_t cmd, uint8_t v)
{
    HAL_GPIO_WritePin(DS1302_CE_PORT, DS1302_CE_PIN, GPIO_PIN_SET);
    write_byte(cmd);
    write_byte(v);
    HAL_GPIO_WritePin(DS1302_CE_PORT, DS1302_CE_PIN, GPIO_PIN_RESET);
}
static void rtc_debug_dump(void)
{
    uint8_t sec   = read_reg(CMD_SEC_R);
    uint8_t min   = read_reg(CMD_MIN_R);
    uint8_t hour  = read_reg(CMD_HOUR_R);
    uint8_t date  = read_reg(CMD_DATE_R);
    uint8_t month = read_reg(CMD_MONTH_R);
    uint8_t day   = read_reg(CMD_DAY_R);
    uint8_t year  = read_reg(CMD_YEAR_R);
    uint8_t ctrl  = read_reg(CMD_CTRL_R);

    char s[128];

    snprintf(s, sizeof(s),
        "[RTC RAW] SEC=%02X MIN=%02X HOUR=%02X DATE=%02X MON=%02X DAY=%02X YEAR=%02X CTRL=%02X\r\n",
        sec, min, hour, date, month, day, year, ctrl);

    BSP_UART_Write(s);
}
HAL_StatusTypeDef DS1302_Init(void)
{
    HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DS1302_CE_PORT, DS1302_CE_PIN, GPIO_PIN_RESET);
    io_output();
    HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN, GPIO_PIN_RESET);

    /* Disable write protection before any clock write. */
    write_reg(CMD_CTRL_W, 0x00);

    /* Clear CH bit if oscillator was stopped. */
    uint8_t sec = read_reg(CMD_SEC_R);
    if (sec & 0x80U) {
        write_reg(CMD_SEC_W, sec & 0x7FU);
    }

    return HAL_OK;
}

HAL_StatusTypeDef DS1302_GetDateTime(DS1302_DateTime *dt)
{
    if (!dt) return HAL_ERROR;

    uint8_t sec = read_reg(CMD_SEC_R);
    uint8_t min = read_reg(CMD_MIN_R);
    uint8_t hour = read_reg(CMD_HOUR_R);
    uint8_t date = read_reg(CMD_DATE_R);
    uint8_t mon = read_reg(CMD_MONTH_R);
    uint8_t year = read_reg(CMD_YEAR_R);

    dt->second = bcd2bin(sec & 0x7F);
    dt->minute = bcd2bin(min & 0x7F);
    if (hour & 0x80U) {
        /* 12-hour mode: bit 5 is PM, bits 4..0 are BCD hour. */
        uint8_t h12 = bcd2bin(hour & 0x1FU);
        dt->hour = (uint8_t)(h12 % 12U);
        if (hour & 0x20U) dt->hour = (uint8_t)(dt->hour + 12U);
    } else {
        /* 24-hour mode. */
        dt->hour = bcd2bin(hour & 0x3FU);
    }
    dt->day = bcd2bin(date & 0x3F);
    dt->month = bcd2bin(mon & 0x1F);
    dt->year = (uint16_t)(2000U + bcd2bin(year));

    if (dt->month < 1 || dt->month > 12 ||
        dt->day < 1 || dt->day > 31 ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef DS1302_SetDateTime(const DS1302_DateTime *dt)
{
    if (!dt || dt->year < 2000 || dt->year > 2099 ||
        dt->month < 1 || dt->month > 12 || dt->day < 1 || dt->day > 31 ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59)
        return HAL_ERROR;

    write_reg(CMD_CTRL_W, 0x00);
    write_reg(CMD_SEC_W, bin2bcd(dt->second) & 0x7F);
    write_reg(CMD_MIN_W, bin2bcd(dt->minute));
    write_reg(CMD_HOUR_W, bin2bcd(dt->hour));
    write_reg(CMD_DATE_W, bin2bcd(dt->day));
    write_reg(CMD_MONTH_W, bin2bcd(dt->month));
    write_reg(CMD_YEAR_W, bin2bcd((uint8_t)(dt->year - 2000U)));
    write_reg(CMD_CTRL_W, 0x80);

    rtc_debug_dump();

    return HAL_OK;
}
