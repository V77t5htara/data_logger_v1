#include "bme280.h"
#include "bsp/bsp.h"
#include "app_config.h"

#define BME280_REG_ID       0xD0
#define BME280_REG_RESET    0xE0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS   0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG   0xF5
#define BME280_REG_DATA     0xF7
#define BME280_REG_CALIB_TP 0x88
#define BME280_REG_CALIB_H1 0xA1
#define BME280_REG_CALIB_H2 0xE1

#define BME280_CHIP_ID 0x60

typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} Calib;

static Calib c;
static int32_t t_fine;

static uint16_t u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static int16_t s16(const uint8_t *p) { return (int16_t)u16(p); }

static HAL_StatusTypeDef rd(uint8_t reg, uint8_t *buf, uint16_t n)
{
    return HAL_I2C_Mem_Read(&hi2c1, BME280_I2C_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, n, 100);
}
static HAL_StatusTypeDef wr(uint8_t reg, uint8_t v)
{
    return HAL_I2C_Mem_Write(&hi2c1, BME280_I2C_ADDRESS, reg,
                             I2C_MEMADD_SIZE_8BIT, &v, 1, 100);
}

static HAL_StatusTypeDef read_calib(void)
{
    uint8_t a[24], h1, h[7];
    if (rd(BME280_REG_CALIB_TP, a, 24) != HAL_OK) return HAL_ERROR;
    if (rd(BME280_REG_CALIB_H1, &h1, 1) != HAL_OK) return HAL_ERROR;
    if (rd(BME280_REG_CALIB_H2, h, 7) != HAL_OK) return HAL_ERROR;

    c.T1=u16(&a[0]); c.T2=s16(&a[2]); c.T3=s16(&a[4]);
    c.P1=u16(&a[6]); c.P2=s16(&a[8]); c.P3=s16(&a[10]); c.P4=s16(&a[12]);
    c.P5=s16(&a[14]); c.P6=s16(&a[16]); c.P7=s16(&a[18]); c.P8=s16(&a[20]); c.P9=s16(&a[22]);
    c.H1=h1; c.H2=s16(&h[0]); c.H3=h[2];
    c.H4=(int16_t)(((int16_t)h[3] << 4) | (h[4] & 0x0F));
    c.H5=(int16_t)(((int16_t)h[5] << 4) | (h[4] >> 4));
    c.H6=(int8_t)h[6];
    return HAL_OK;
}

static int32_t comp_temp(int32_t adc)
{
    int32_t v1 = ((((adc >> 3) - ((int32_t)c.T1 << 1))) * c.T2) >> 11;
    int32_t v2 = (((((adc >> 4) - c.T1) * ((adc >> 4) - c.T1)) >> 12) * c.T3) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8; /* 0.01 C */
}

static uint32_t comp_press(int32_t adc)
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * c.P6;
    v2 += (v1 * c.P5) << 17;
    v2 += ((int64_t)c.P4) << 35;
    v1 = ((v1 * v1 * c.P3) >> 8) + ((v1 * c.P2) << 12);
    v1 = (((int64_t)1 << 47) + v1) * c.P1 >> 33;
    if (v1 == 0) return 0;
    int64_t p = 1048576 - adc;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = ((int64_t)c.P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)c.P8 * p) >> 19;
    p = ((p + v1 + v2) >> 8) + ((int64_t)c.P7 << 4);
    return (uint32_t)(p / 256); /* Pa */
}

static uint32_t comp_hum(int32_t adc)
{
    int64_t v = (int64_t)t_fine - 76800;
    int64_t a = ((((int64_t)adc << 14) -
                  ((int64_t)c.H4 << 20) -
                  ((int64_t)c.H5 * v) + 16384) >> 15);
    int64_t b = (((((v * c.H6) >> 10) *
                   (((v * c.H3) >> 11) + 32768)) >> 10) + 2097152);
    b = (b * c.H2 + 8192) >> 14;
    v = a * b;
    v -= (((((v >> 15) * (v >> 15)) >> 7) * c.H1) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12); /* %RH * 1024 */
}

HAL_StatusTypeDef BME280_Init(void)
{
    uint8_t id;
    if (rd(BME280_REG_ID, &id, 1) != HAL_OK || id != BME280_CHIP_ID) return HAL_ERROR;
    if (read_calib() != HAL_OK) return HAL_ERROR;

    /* humidity x1; temperature x1; pressure x1; normal mode */
    if (wr(BME280_REG_CTRL_HUM, 0x01) != HAL_OK) return HAL_ERROR;
    if (wr(BME280_REG_CTRL_MEAS, 0x27) != HAL_OK) return HAL_ERROR;
    /* standby 1 s, filter off */
    if (wr(BME280_REG_CONFIG, 0xA0) != HAL_OK) return HAL_ERROR;
    HAL_Delay(10);
    return HAL_OK;
}

HAL_StatusTypeDef BME280_ReadData(BME280_Data *data)
{
    uint8_t b[8];
    if (!data) return HAL_ERROR;
    if (rd(BME280_REG_DATA, b, 8) != HAL_OK) return HAL_ERROR;

    int32_t p_raw = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | (b[2] >> 4);
    int32_t t_raw = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | (b[5] >> 4);
    int32_t h_raw = ((int32_t)b[6] << 8) | b[7];

    data->temperature_c_x100 = comp_temp(t_raw);
    data->pressure_pa = comp_press(p_raw);
    data->humidity_rh_x1000 = (comp_hum(h_raw) * 1000U) / 1024U;
    return HAL_OK;
}
