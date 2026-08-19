#include "sd_spi.h"
#include "bsp/bsp.h"
#include "app_config.h"

#define CMD0   0
#define CMD8   8
#define CMD9   9
#define CMD12 12
#define CMD16 16
#define CMD17 17
#define CMD24 24
#define CMD55 55
#define CMD58 58
#define ACMD41 41

static SD_Type type = SD_TYPE_UNKNOWN;
static uint32_t sectors = 0;

static uint8_t spi(uint8_t v)
{
    uint8_t r = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &v, &r, 1, 100);
    return r;
}

static void cs(uint8_t active)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_PORT, SD_CS_PIN,
                      active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void clocks(uint16_t n)
{
    while (n--) spi(0xFF);
}

static uint8_t wait_ready(uint32_t timeout_ms)
{
    uint32_t t = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t) < timeout_ms) {
        if (spi(0xFF) == 0xFF) return 1;
    }
    return 0;
}

static uint8_t command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r;

    cs(1);
    spi(0xFF);

    spi(0x40 | cmd);
    spi((uint8_t)(arg >> 24));
    spi((uint8_t)(arg >> 16));
    spi((uint8_t)(arg >> 8));
    spi((uint8_t)arg);
    spi(crc);

    for (uint8_t i = 0; i < 10; ++i) {
        r = spi(0xFF);
        if (!(r & 0x80)) return r;
    }
    return 0xFF;
}

static uint8_t acmd(uint8_t cmd, uint32_t arg)
{
    uint8_t r = command(CMD55, 0, 0x01);
    cs(0);
    if (r > 1) return r;
    cs(1);
    return command(cmd, arg, 0x01);
}

static void deselect(void)
{
    cs(0);
    spi(0xFF);
}

static void spi_fast(void)
{
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    HAL_SPI_Init(&hspi1);
}

static void spi_slow(void)
{
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    HAL_SPI_Init(&hspi1);
}

HAL_StatusTypeDef SD_Init(void)
{
    uint8_t r;
    uint8_t ocr[4];
    uint8_t csd[16];

    type = SD_TYPE_UNKNOWN;
    sectors = 0;

    spi_slow();
    cs(0);
    clocks(10);

    /* CMD0 */
    for (uint8_t i = 0; i < 20; ++i) {
    r = command(CMD0, 0, 0x95);

    char msg[64];
    snprintf(msg, sizeof(msg),
             "[SD] CMD0 attempt=%u R1=0x%02X\r\n",
             i + 1, r);
    BSP_UART_Write(msg);

    deselect();

    if (r == 0x01) {
        BSP_UART_WriteLine("[SD] CMD0 OK");
        break;
    }

    if (i == 19) {
        BSP_UART_WriteLine("[SD] CMD0 FAILED");
        return HAL_ERROR;
    }
}

    /* CMD8: SD v2 */
    r = command(CMD8, 0x1AA, 0x87);
    if (r == 0x01) {
        for (uint8_t i = 0; i < 4; ++i) ocr[i] = spi(0xFF);
        deselect();

        if (ocr[2] != 0x01 || ocr[3] != 0xAA) return HAL_ERROR;

        uint32_t t = HAL_GetTick();
        do {
            r = acmd(ACMD41, 0x40000000);
            deselect();
            if ((uint32_t)(HAL_GetTick() - t) > 2000) return HAL_TIMEOUT;
            HAL_Delay(1);
        } while (r != 0x00);

        r = command(CMD58, 0, 0x01);
        if (r != 0x00) {
            deselect();
            return HAL_ERROR;
        }
        for (uint8_t i = 0; i < 4; ++i) ocr[i] = spi(0xFF);
        deselect();

        type = (ocr[0] & 0x40) ? SD_TYPE_SDHC : SD_TYPE_SDSC;
    } else {
        /* Legacy SDSC/MMC path: support SDSC cards. */
        deselect();

        uint32_t t = HAL_GetTick();
        do {
            r = acmd(ACMD41, 0);
            deselect();
            if ((uint32_t)(HAL_GetTick() - t) > 2000) return HAL_TIMEOUT;
            HAL_Delay(1);
        } while (r != 0x00);

        type = SD_TYPE_SDSC;
        if (command(CMD16, 512, 0x01) != 0x00) {
            deselect();
            return HAL_ERROR;
        }
        deselect();
    }

    /* Read CSD to determine capacity. */
    r = command(CMD9, 0, 0x01);
    if (r != 0x00) {
        deselect();
        return HAL_ERROR;
    }

    uint32_t t = HAL_GetTick();
    while (spi(0xFF) != 0xFE) {
        if ((uint32_t)(HAL_GetTick() - t) > 500) {
            deselect();
            return HAL_TIMEOUT;
        }
    }

    for (uint8_t i = 0; i < 16; ++i) csd[i] = spi(0xFF);
    spi(0xFF); spi(0xFF);
    deselect();

    if (type == SD_TYPE_SDHC) {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                          ((uint32_t)csd[8] << 8) | csd[9];
        sectors = (c_size + 1U) * 1024U;
    } else {
        uint8_t read_bl_len = csd[5] & 0x0F;
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10) |
                          ((uint32_t)csd[7] << 2) |
                          ((csd[8] >> 6) & 0x03);
        uint8_t c_size_mult = (uint8_t)(((csd[9] & 0x03) << 1) |
                                        ((csd[10] >> 7) & 0x01));
        uint32_t block_len = 1UL << read_bl_len;
        uint32_t mult = 1UL << (c_size_mult + 2U);
        uint64_t capacity = (uint64_t)(c_size + 1U) * mult * block_len;
        sectors = (uint32_t)(capacity / 512ULL);
    }

    spi_fast();
    return sectors ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef SD_ReadBlock(uint8_t *buf, uint32_t sector)
{
    if (!buf || type == SD_TYPE_UNKNOWN) return HAL_ERROR;

    uint32_t addr = (type == SD_TYPE_SDHC) ? sector : sector * 512UL;
    uint8_t r = command(CMD17, addr, 0x01);
    if (r != 0x00) {
        deselect();
        return HAL_ERROR;
    }

    uint32_t t = HAL_GetTick();
    while (spi(0xFF) != 0xFE) {
        if ((uint32_t)(HAL_GetTick() - t) > 500) {
            deselect();
            return HAL_TIMEOUT;
        }
    }

    for (uint16_t i = 0; i < 512; ++i) buf[i] = spi(0xFF);
    spi(0xFF); spi(0xFF);
    deselect();
    return HAL_OK;
}

HAL_StatusTypeDef SD_WriteBlock(const uint8_t *buf, uint32_t sector)
{
    if (!buf || type == SD_TYPE_UNKNOWN) return HAL_ERROR;

    uint32_t addr = (type == SD_TYPE_SDHC) ? sector : sector * 512UL;
    uint8_t r = command(CMD24, addr, 0x01);
    if (r != 0x00) {
        deselect();
        return HAL_ERROR;
    }

    spi(0xFF);
    spi(0xFE);

    for (uint16_t i = 0; i < 512; ++i) spi(buf[i]);
    spi(0xFF); spi(0xFF);

    r = spi(0xFF);
    if ((r & 0x1F) != 0x05) {
        deselect();
        return HAL_ERROR;
    }

    if (!wait_ready(500)) {
        deselect();
        return HAL_TIMEOUT;
    }

    deselect();
    return HAL_OK;
}

uint32_t SD_GetSectorCount(void) { return sectors; }
uint8_t SD_IsReady(void) { return type != SD_TYPE_UNKNOWN; }
