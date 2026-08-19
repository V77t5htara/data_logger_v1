#include "logger.h"
#include "app_config.h"
#include "bsp/bsp.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

static FATFS fs;
static FIL file;
static uint8_t mounted;
static uint8_t opened;

HAL_StatusTypeDef Logger_Init(void)
{
    FRESULT r;

    r = f_mount(&fs, "", 1);
    if (r != FR_OK) {
        BSP_UART_WriteLine("[ERROR] FATFS mount failed");
        return HAL_ERROR;
    }
    mounted = 1;

    r = f_open(&file, LOG_FILE, FA_OPEN_ALWAYS | FA_WRITE);
    if (r != FR_OK) {
        BSP_UART_WriteLine("[ERROR] log file open failed");
        return HAL_ERROR;
    }
    opened = 1;

    if (f_size(&file) == 0) {
        static const char header[] =
            "timestamp,sequence,temperature_c,humidity_percent,pressure_pa\r\n";
        UINT bw = 0;
        r = f_write(&file, header, (UINT)(sizeof(header) - 1), &bw);
        if (r != FR_OK || bw != sizeof(header) - 1) {
            BSP_UART_WriteLine("[ERROR] log header write failed");
            return HAL_ERROR;
        }
        if (f_sync(&file) != FR_OK) {
            BSP_UART_WriteLine("[ERROR] log sync failed");
            return HAL_ERROR;
        }
    }

    r = f_lseek(&file, f_size(&file));
    if (r != FR_OK) {
        BSP_UART_WriteLine("[ERROR] log seek failed");
        return HAL_ERROR;
    }

    BSP_UART_WriteLine("[INIT] FAT32/CSV logger OK");
    return HAL_OK;
}

HAL_StatusTypeDef Logger_Write(const DS1302_DateTime *dt,
                               const BME280_Data *data,
                               uint32_t sequence)
{
    if (!opened || !dt || !data) return HAL_ERROR;

    char line[128];
    int32_t tc = data->temperature_c_x100;
    uint32_t t_abs = (tc < 0) ? (uint32_t)(-tc) : (uint32_t)tc;
    uint32_t h_int = data->humidity_rh_x1000 / 1000U;
    uint32_t h_frac = (data->humidity_rh_x1000 % 1000U) / 10U;

    int n = snprintf(line, sizeof(line),
        "%04u-%02u-%02uT%02u:%02u:%02u,%lu,%s%lu.%02lu,%lu.%02lu,%lu\r\n",
        dt->year, dt->month, dt->day,
        dt->hour, dt->minute, dt->second,
        (unsigned long)sequence,
        (tc < 0) ? "-" : "",
        (unsigned long)(t_abs / 100U),
        (unsigned long)(t_abs % 100U),
        (unsigned long)h_int,
        (unsigned long)h_frac,
        (unsigned long)data->pressure_pa);

    if (n <= 0 || n >= (int)sizeof(line)) return HAL_ERROR;

    UINT bw = 0;
    FRESULT r = f_write(&file, line, (UINT)n, &bw);
    if (r != FR_OK || bw != (UINT)n) return HAL_ERROR;

    /* V1 is powered from USB, so prioritize durability over throughput. */
    if (f_sync(&file) != FR_OK) return HAL_ERROR;

    return HAL_OK;
}

void Logger_Close(void)
{
    if (opened) {
        f_sync(&file);
        f_close(&file);
        opened = 0;
    }
    if (mounted) {
        f_mount(NULL, "", 0);
        mounted = 0;
    }
}
