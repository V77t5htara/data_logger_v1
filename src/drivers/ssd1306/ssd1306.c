#include "ssd1306.h"
#include "bsp/bsp.h"
#include "app_config.h"
#include <string.h>

static uint8_t fb[1024];

static HAL_StatusTypeDef cmd(uint8_t c)
{
    uint8_t p[2] = {0x00, c};
    return HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDRESS, p, 2, 100);
}

static HAL_StatusTypeDef data(const uint8_t *d, uint16_t n)
{
    /* 1 control byte + up to 128 data bytes */
    uint8_t p[129];
    p[0] = 0x40;
    memcpy(&p[1], d, n);
    return HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDRESS, p, (uint16_t)(n + 1), 100);
}

static const uint8_t *glyph(char ch)
{
    static uint8_t g[5];
    memset(g, 0, sizeof(g));

    switch (ch) {
    case '0': g[0]=0x3E;g[1]=0x51;g[2]=0x49;g[3]=0x45;g[4]=0x3E; break;
    case '1': g[0]=0x00;g[1]=0x42;g[2]=0x7F;g[3]=0x40;g[4]=0x00; break;
    case '2': g[0]=0x42;g[1]=0x61;g[2]=0x51;g[3]=0x49;g[4]=0x46; break;
    case '3': g[0]=0x21;g[1]=0x41;g[2]=0x45;g[3]=0x4B;g[4]=0x31; break;
    case '4': g[0]=0x18;g[1]=0x14;g[2]=0x12;g[3]=0x7F;g[4]=0x10; break;
    case '5': g[0]=0x27;g[1]=0x45;g[2]=0x45;g[3]=0x45;g[4]=0x39; break;
    case '6': g[0]=0x3C;g[1]=0x4A;g[2]=0x49;g[3]=0x49;g[4]=0x30; break;
    case '7': g[0]=0x01;g[1]=0x71;g[2]=0x09;g[3]=0x05;g[4]=0x03; break;
    case '8': g[0]=0x36;g[1]=0x49;g[2]=0x49;g[3]=0x49;g[4]=0x36; break;
    case '9': g[0]=0x06;g[1]=0x49;g[2]=0x49;g[3]=0x29;g[4]=0x1E; break;
    case 'A': g[0]=0x7E;g[1]=0x11;g[2]=0x11;g[3]=0x11;g[4]=0x7E; break;
    case 'C': g[0]=0x3E;g[1]=0x41;g[2]=0x41;g[3]=0x41;g[4]=0x22; break;
    case 'D': g[0]=0x7F;g[1]=0x41;g[2]=0x41;g[3]=0x22;g[4]=0x1C; break;
    case 'E': g[0]=0x7F;g[1]=0x49;g[2]=0x49;g[3]=0x49;g[4]=0x41; break;
    case 'G': g[0]=0x3E;g[1]=0x41;g[2]=0x49;g[3]=0x49;g[4]=0x7A; break;
    case 'H': g[0]=0x7F;g[1]=0x08;g[2]=0x08;g[3]=0x08;g[4]=0x7F; break;
    case 'L': g[0]=0x7F;g[1]=0x40;g[2]=0x40;g[3]=0x40;g[4]=0x40; break;
    case 'O': g[0]=0x3E;g[1]=0x41;g[2]=0x41;g[3]=0x41;g[4]=0x3E; break;
    case 'P': g[0]=0x7F;g[1]=0x09;g[2]=0x09;g[3]=0x09;g[4]=0x06; break;
    case 'R': g[0]=0x7F;g[1]=0x09;g[2]=0x19;g[3]=0x29;g[4]=0x46; break;
    case 'S': g[0]=0x46;g[1]=0x49;g[2]=0x49;g[3]=0x49;g[4]=0x31; break;
    case 'T': g[0]=0x01;g[1]=0x01;g[2]=0x7F;g[3]=0x01;g[4]=0x01; break;
    case 'V': g[0]=0x1F;g[1]=0x20;g[2]=0x40;g[3]=0x20;g[4]=0x1F; break;
    case 'K': g[0]=0x7F;g[1]=0x08;g[2]=0x14;g[3]=0x22;g[4]=0x41; break;
    case ' ': break;
    case ':': g[0]=0x00;g[1]=0x36;g[2]=0x36;g[3]=0x00;g[4]=0x00; break;
    case '.': g[0]=0x00;g[1]=0x60;g[2]=0x60;g[3]=0x00;g[4]=0x00; break;
    case '%': g[0]=0x63;g[1]=0x13;g[2]=0x08;g[3]=0x64;g[4]=0x63; break;
    case '-': g[0]=0x08;g[1]=0x08;g[2]=0x08;g[3]=0x08;g[4]=0x08; break;
    case '/': g[0]=0x20;g[1]=0x10;g[2]=0x08;g[3]=0x04;g[4]=0x02; break;
    default: break;
    }
    return g;
}

HAL_StatusTypeDef SSD1306_Init(void)
{
    const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (uint32_t i = 0; i < sizeof(init); ++i)
        if (cmd(init[i]) != HAL_OK) return HAL_ERROR;
    SSD1306_Clear();
    return SSD1306_Update();
}

void SSD1306_Clear(void)
{
    memset(fb, 0, sizeof(fb));
}

void SSD1306_DrawString(uint8_t x, uint8_t y, const char *s)
{
    if (y > 56) return;
    while (*s && x < 123) {
        const uint8_t *g = glyph(*s++);
        for (uint8_t col = 0; col < 5; ++col)
            fb[(uint16_t)y * 128U + x + col] = g[col];
        x += 6;
    }
}

HAL_StatusTypeDef SSD1306_Update(void)
{
    for (uint8_t page = 0; page < 8; ++page) {
        if (cmd((uint8_t)(0xB0 | page)) != HAL_OK) return HAL_ERROR;
        if (cmd(0x00) != HAL_OK) return HAL_ERROR;
        if (cmd(0x10) != HAL_OK) return HAL_ERROR;
        if (data(&fb[(uint16_t)page * 128U], 128) != HAL_OK) return HAL_ERROR;
    }
    return HAL_OK;
}
