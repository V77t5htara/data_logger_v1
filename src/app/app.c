#include "app.h"
#include "app_config.h"
#include "version.h"

#include "bsp/bsp.h"
#include "drivers/bme280/bme280.h"
#include "drivers/ds1302/ds1302.h"
#include "drivers/ssd1306/ssd1306.h"
#include "drivers/sd/sd_spi.h"
#include "services/scheduler.h"
#include "services/logger.h"
#include "services/error_manager.h"

#include <stdio.h>

static BME280_Data sensor_data;
static DS1302_DateTime rtc_data;
static uint32_t sequence;
static uint8_t bme_ok;
static uint8_t rtc_ok;
static uint8_t oled_ok;
static uint8_t sd_ok;
static uint8_t logger_ok;

static void print_version(void)
{
    char s[64];
    snprintf(s, sizeof(s),
             "STM32 Data Logger V%d.%d.%d\r\n",
             LOGGER_VERSION_MAJOR,
             LOGGER_VERSION_MINOR,
             LOGGER_VERSION_PATCH);
    BSP_UART_Write(s);
}

static void task_sample(void)
{
    if (BME280_ReadData(&sensor_data) != HAL_OK) {
        ErrorManager_Set(ERR_BME280);
        bme_ok = 0;
        BSP_UART_WriteLine("[ERROR] BME280 read");
        return;
    }
    bme_ok = 1;

    if (DS1302_GetDateTime(&rtc_data) != HAL_OK) {
        ErrorManager_Set(ERR_RTC);
        rtc_ok = 0;
        BSP_UART_WriteLine("[ERROR] RTC read");
        return;
    }
    rtc_ok = 1;

    ++sequence;

    char s[128];
    uint32_t tabs = (sensor_data.temperature_c_x100 < 0) ?
                    (uint32_t)(-sensor_data.temperature_c_x100) :
                    (uint32_t)sensor_data.temperature_c_x100;

    snprintf(s, sizeof(s),
        "[DATA] %04u-%02u-%02uT%02u:%02u:%02u seq=%lu T=%s%lu.%02luC H=%lu.%02lu%% P=%luPa\r\n",
        rtc_data.year, rtc_data.month, rtc_data.day,
        rtc_data.hour, rtc_data.minute, rtc_data.second,
        (unsigned long)sequence,
        (sensor_data.temperature_c_x100 < 0) ? "-" : "",
        (unsigned long)(tabs / 100U),
        (unsigned long)(tabs % 100U),
        (unsigned long)(sensor_data.humidity_rh_x1000 / 1000U),
        (unsigned long)((sensor_data.humidity_rh_x1000 % 1000U) / 10U),
        (unsigned long)sensor_data.pressure_pa);
    BSP_UART_Write(s);

    if (logger_ok) {
        if (Logger_Write(&rtc_data, &sensor_data, sequence) != HAL_OK) {
            ErrorManager_Set(ERR_LOGGER);
            logger_ok = 0;
            BSP_UART_WriteLine("[ERROR] logger write");
        }
    }
}

static void task_display(void)
{
    if (!oled_ok) return;

    char line[24];
    uint32_t tabs = (sensor_data.temperature_c_x100 < 0) ?
                    (uint32_t)(-sensor_data.temperature_c_x100) :
                    (uint32_t)sensor_data.temperature_c_x100;

    SSD1306_Clear();

    SSD1306_DrawString(0, 0, "LOGGER V1");

    snprintf(line, sizeof(line), "T:%s%lu.%02lu C",
             (sensor_data.temperature_c_x100 < 0) ? "-" : "",
             (unsigned long)(tabs / 100U),
             (unsigned long)(tabs % 100U));
    SSD1306_DrawString(0, 2, line);

    snprintf(line, sizeof(line), "H:%lu.%02lu %%",
             (unsigned long)(sensor_data.humidity_rh_x1000 / 1000U),
             (unsigned long)((sensor_data.humidity_rh_x1000 % 1000U) / 10U));
    SSD1306_DrawString(0, 3, line);

    snprintf(line, sizeof(line), "P:%lu PA",
             (unsigned long)sensor_data.pressure_pa);
    SSD1306_DrawString(0, 4, line);

    SSD1306_DrawString(0, 6, logger_ok ? "SD:OK" : "SD:ERR");
    SSD1306_Update();
}

static void task_system(void)
{
    if (ErrorManager_Get() == 0)
        BSP_UART_WriteLine("[SYSTEM] OK");
}

void App_Init(void)
{
    BSP_GPIO_Init();
    BSP_UART1_Init();
    BSP_I2C1_Init();
    BSP_SPI1_Init();

    print_version();
    BSP_UART_WriteLine("[INIT] BSP OK");

    if (BME280_Init() == HAL_OK) {
        bme_ok = 1;
        BSP_UART_WriteLine("[INIT] BME280 OK");
    } else {
        ErrorManager_Set(ERR_BME280);
        BSP_UART_WriteLine("[ERROR] BME280 init");
    }

    if (DS1302_Init() == HAL_OK) {

    DS1302_DateTime def = {
        RTC_DEFAULT_YEAR,
        RTC_DEFAULT_MONTH,
        RTC_DEFAULT_DAY,
        RTC_DEFAULT_HOUR,
        RTC_DEFAULT_MINUTE,
        RTC_DEFAULT_SECOND
    };

    char msg[128];

    snprintf(msg, sizeof(msg),
             "[RTC] SET %04u-%02u-%02u %02u:%02u:%02u\r\n",
             def.year,
             def.month,
             def.day,
             def.hour,
             def.minute,
             def.second);
    BSP_UART_Write(msg);

    if (DS1302_SetDateTime(&def) != HAL_OK) {
    ErrorManager_Set(ERR_RTC);
    BSP_UART_WriteLine("[ERROR] RTC SET");
} else {
    BSP_UART_WriteLine("[RTC] SET OK");

    DS1302_DateTime check = {0};

        if (DS1302_GetDateTime(&check) != HAL_OK) {
            ErrorManager_Set(ERR_RTC);
            BSP_UART_WriteLine("[RTC] READBACK FAILED");
        } else {
            snprintf(msg, sizeof(msg),
                     "[RTC] READBACK %04u-%02u-%02u %02u:%02u:%02u\r\n",
                     check.year,
                     check.month,
                     check.day,
                     check.hour,
                     check.minute,
                     check.second);
            BSP_UART_Write(msg);

            if (check.year   != def.year   ||
                check.month  != def.month  ||
                check.day    != def.day    ||
                check.hour   != def.hour   ||
                check.minute != def.minute ||
                check.second != def.second) {

                BSP_UART_WriteLine("[RTC] MISMATCH!");
                ErrorManager_Set(ERR_RTC);
            } else {
                rtc_data = check;
                rtc_ok = 1;
                BSP_UART_WriteLine("[RTC] WRITE/READ OK");
            }
        }
    }

} else {
    ErrorManager_Set(ERR_RTC);
    BSP_UART_WriteLine("[ERROR] RTC INIT");
}

    if (SSD1306_Init() == HAL_OK) {
        oled_ok = 1;
        BSP_UART_WriteLine("[INIT] OLED OK");
    } else {
        ErrorManager_Set(ERR_OLED);
        BSP_UART_WriteLine("[ERROR] OLED init");
    }

    if (SD_Init() == HAL_OK) {
        sd_ok = 1;
        BSP_UART_WriteLine("[INIT] SD OK");
        char s[64];
        snprintf(s, sizeof(s), "[INIT] SD sectors=%lu\r\n",
                 (unsigned long)SD_GetSectorCount());
        BSP_UART_Write(s);
    } else {
        ErrorManager_Set(ERR_SD);
        BSP_UART_WriteLine("[ERROR] SD init");
    }

    if (sd_ok && rtc_ok) {
        if (Logger_Init() == HAL_OK) {
            logger_ok = 1;
        } else {
            ErrorManager_Set(ERR_FATFS | ERR_LOGGER);
        }
    } else {
        ErrorManager_Set(ERR_FATFS | ERR_LOGGER);
        BSP_UART_WriteLine("[ERROR] logger skipped");
    }

    Scheduler_Init();
    Scheduler_Add(task_sample, LOGGER_SAMPLE_PERIOD_MS);
    Scheduler_Add(task_display, DISPLAY_PERIOD_MS);
    Scheduler_Add(task_system, SYSTEM_PERIOD_MS);

    BSP_UART_WriteLine("[SYSTEM] RUNNING");
}

void App_Run(void)
{
    Scheduler_Run();
}
