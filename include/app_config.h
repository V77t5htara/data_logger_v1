#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define LOGGER_SAMPLE_PERIOD_MS     1000U
#define DISPLAY_PERIOD_MS            500U
#define SYSTEM_PERIOD_MS            1000U

#define BME280_I2C_ADDRESS          (0x76U << 1)
#define SSD1306_I2C_ADDRESS         (0x3CU << 1)

#define SD_CS_GPIO_PORT             GPIOA
#define SD_CS_PIN                   GPIO_PIN_4

#define DS1302_CLK_PORT             GPIOB
#define DS1302_CLK_PIN              GPIO_PIN_12
#define DS1302_IO_PORT              GPIOB
#define DS1302_IO_PIN               GPIO_PIN_13
#define DS1302_CE_PORT              GPIOB
#define DS1302_CE_PIN               GPIO_PIN_14

#define LOG_FILE                    "LOG0001.CSV"

#define RTC_DEFAULT_ON_INVALID      1U
#define RTC_DEFAULT_YEAR            2026U
#define RTC_DEFAULT_MONTH           8U
#define RTC_DEFAULT_DAY             19U
#define RTC_DEFAULT_HOUR            13U
#define RTC_DEFAULT_MINUTE          36U
#define RTC_DEFAULT_SECOND          0U

#endif
