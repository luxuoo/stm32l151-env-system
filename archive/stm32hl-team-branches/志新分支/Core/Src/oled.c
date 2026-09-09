#include "oled.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ======================================================================== */
/*                              字库                                         */
/* ======================================================================== */

/* 6x8 ASCII 字库,从空格(0x20)开始,LSB 在顶部 */
static const uint8_t Font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // 0x20 (space)
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, // #
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, // $
    {0x23,0x13,0x08,0x64,0x62,0x00}, // %
    {0x36,0x49,0x55,0x22,0x50,0x00}, // &
    {0x00,0x05,0x03,0x00,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14,0x00}, // *
    {0x08,0x08,0x3E,0x08,0x08,0x00}, // +
    {0x00,0x50,0x30,0x00,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08,0x00}, // -
    {0x00,0x60,0x60,0x00,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02,0x00}, // /
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, // 0
    {0x00,0x42,0x7F,0x40,0x00,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46,0x00}, // 2
    {0x21,0x41,0x45,0x4B,0x31,0x00}, // 3
    {0x18,0x14,0x12,0x7F,0x10,0x00}, // 4
    {0x27,0x45,0x45,0x45,0x39,0x00}, // 5
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, // 6
    {0x01,0x71,0x09,0x05,0x03,0x00}, // 7
    {0x36,0x49,0x49,0x49,0x36,0x00}, // 8
    {0x06,0x49,0x49,0x29,0x1E,0x00}, // 9
    {0x00,0x36,0x36,0x00,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14,0x00}, // =
    {0x00,0x41,0x22,0x14,0x08,0x00}, // >
    {0x02,0x01,0x51,0x09,0x06,0x00}, // ?
    {0x32,0x49,0x79,0x41,0x3E,0x00}, // @
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, // A
    {0x7F,0x49,0x49,0x49,0x36,0x00}, // B
    {0x3E,0x41,0x41,0x41,0x22,0x00}, // C
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, // D
    {0x7F,0x49,0x49,0x49,0x41,0x00}, // E
    {0x7F,0x09,0x09,0x09,0x01,0x00}, // F
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, // G
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, // H
    {0x00,0x41,0x7F,0x41,0x00,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01,0x00}, // J
    {0x7F,0x08,0x14,0x22,0x41,0x00}, // K
    {0x7F,0x40,0x40,0x40,0x40,0x00}, // L
    {0x7F,0x02,0x0C,0x02,0x7F,0x00}, // M
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, // N
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, // O
    {0x7F,0x09,0x09,0x09,0x06,0x00}, // P
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, // Q
    {0x7F,0x09,0x19,0x29,0x46,0x00}, // R
    {0x46,0x49,0x49,0x49,0x31,0x00}, // S
    {0x01,0x01,0x7F,0x01,0x01,0x00}, // T
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, // U
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, // V
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, // W
    {0x63,0x14,0x08,0x14,0x63,0x00}, // X
    {0x07,0x08,0x70,0x08,0x07,0x00}, // Y
    {0x61,0x51,0x49,0x45,0x43,0x00}, // Z
    {0x00,0x7F,0x41,0x41,0x00,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04,0x00}, // ^
    {0x40,0x40,0x40,0x40,0x40,0x00}, // _
    {0x00,0x01,0x02,0x04,0x00,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78,0x00}, // a
    {0x7F,0x48,0x44,0x44,0x38,0x00}, // b
    {0x38,0x44,0x44,0x44,0x20,0x00}, // c
    {0x38,0x44,0x44,0x48,0x7F,0x00}, // d
    {0x38,0x54,0x54,0x54,0x18,0x00}, // e
    {0x08,0x7E,0x09,0x01,0x02,0x00}, // f
    {0x0C,0x52,0x52,0x52,0x3E,0x00}, // g
    {0x7F,0x08,0x04,0x04,0x78,0x00}, // h
    {0x00,0x44,0x7D,0x40,0x00,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00,0x00}, // j
    {0x7F,0x10,0x28,0x44,0x00,0x00}, // k
    {0x00,0x41,0x7F,0x40,0x00,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78,0x00}, // m
    {0x7C,0x08,0x04,0x04,0x78,0x00}, // n
    {0x38,0x44,0x44,0x44,0x38,0x00}, // o
    {0x7C,0x14,0x14,0x14,0x08,0x00}, // p
    {0x08,0x14,0x14,0x18,0x7C,0x00}, // q
    {0x7C,0x08,0x04,0x04,0x08,0x00}, // r
    {0x48,0x54,0x54,0x54,0x20,0x00}, // s
    {0x04,0x3F,0x44,0x40,0x20,0x00}, // t
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, // u
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, // v
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, // w
    {0x44,0x28,0x10,0x28,0x44,0x00}, // x
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, // y
    {0x44,0x64,0x54,0x4C,0x44,0x00}, // z
    {0x00,0x08,0x36,0x41,0x00,0x00}, // {
    {0x00,0x00,0x7F,0x00,0x00,0x00}, // |
    {0x00,0x41,0x36,0x08,0x00,0x00}, // }
    {0x08,0x04,0x08,0x10,0x08,0x00}, // ~
};

/* ======================================================================== */
/*                              内部状态                                     */
/* ======================================================================== */

static uint8_t OLED_Buffer[OLED_PAGES][OLED_WIDTH];

/* ======================================================================== */
/*                              底层驱动                                     */
/* ======================================================================== */

static inline void OLED_DC_High(void) { OLED_DC_PORT->BSRR = OLED_DC_PIN; }
static inline void OLED_DC_Low(void)  { OLED_DC_PORT->BSRR = (uint32_t)OLED_DC_PIN << 16; }
static inline void OLED_CS_High(void) { OLED_CS_PORT->BSRR = OLED_CS_PIN; }
static inline void OLED_CS_Low(void)  { OLED_CS_PORT->BSRR = (uint32_t)OLED_CS_PIN << 16; }
static inline void OLED_SCK_High(void){ OLED_SCK_PORT->BSRR = OLED_SCK_PIN; }
static inline void OLED_SCK_Low(void) { OLED_SCK_PORT->BSRR = (uint32_t)OLED_SCK_PIN << 16; }
static inline void OLED_SDA_High(void){ OLED_SDA_PORT->BSRR = OLED_SDA_PIN; }
static inline void OLED_SDA_Low(void) { OLED_SDA_PORT->BSRR = (uint32_t)OLED_SDA_PIN << 16; }

static void OLED_WriteByte(uint8_t dat, uint8_t cmd)
{
    if (cmd) OLED_DC_High();
    else     OLED_DC_Low();

    OLED_CS_Low();

    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_SCK_Low();
        if (dat & 0x80) OLED_SDA_High();
        else            OLED_SDA_Low();
        OLED_SCK_High();
        dat <<= 1;
    }

    OLED_CS_High();
}

static void OLED_SetPos(uint8_t x, uint8_t page)
{
    x += OLED_X_OFFSET;
    OLED_WriteByte(0xB0 + page, OLED_CMD);                  /* 页地址 0xB0~0xB7 */
    OLED_WriteByte(0x10 | ((x >> 4) & 0x0F), OLED_CMD);     /* 列地址高 4 位 */
    OLED_WriteByte(0x00 | (x & 0x0F), OLED_CMD);            /* 列地址低 4 位 */
}

/* ======================================================================== */
/*                               初始化                                      */
/* ======================================================================== */

void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = OLED_CS_PIN | OLED_DC_PIN | OLED_SCK_PIN | OLED_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    OLED_CS_High();
    OLED_DC_High();
    OLED_SCK_High();
    OLED_SDA_High();

    HAL_Delay(100);

    OLED_WriteByte(0xAE, OLED_CMD); /* 关闭显示 */
    OLED_WriteByte(0x20, OLED_CMD); /* 寻址模式 */
    OLED_WriteByte(0x10, OLED_CMD); /* 页寻址 */
    OLED_WriteByte(0xB0, OLED_CMD); /* 起始页 0 */
    OLED_WriteByte(0xC8, OLED_CMD); /* COM 反向扫描 */
    OLED_WriteByte(0x00, OLED_CMD); /* 列地址低 4 位 */
    OLED_WriteByte(0x10, OLED_CMD); /* 列地址高 4 位 */
    OLED_WriteByte(0x40, OLED_CMD); /* 起始行 0 */
    OLED_WriteByte(0x81, OLED_CMD); /* 对比度 */
    OLED_WriteByte(0xCF, OLED_CMD);
    OLED_WriteByte(0xA1, OLED_CMD); /* 段重映射 */
    OLED_WriteByte(0xA6, OLED_CMD); /* 正常显示 */
    OLED_WriteByte(0xA8, OLED_CMD); /* 多路复用 */
    OLED_WriteByte(0x3F, OLED_CMD); /* 1/64 占空比 */
    OLED_WriteByte(0xA4, OLED_CMD); /* 全部点亮取消 */
    OLED_WriteByte(0xD3, OLED_CMD); /* 显示偏移 */
    OLED_WriteByte(0x00, OLED_CMD);
    OLED_WriteByte(0xD5, OLED_CMD); /* 时钟分频 */
    OLED_WriteByte(0xF0, OLED_CMD);
    OLED_WriteByte(0xD9, OLED_CMD); /* 预充电 */
    OLED_WriteByte(0x22, OLED_CMD);
    OLED_WriteByte(0xDA, OLED_CMD); /* COM 引脚 */
    OLED_WriteByte(0x12, OLED_CMD);
    OLED_WriteByte(0xDB, OLED_CMD); /* VCOMH */
    OLED_WriteByte(0x20, OLED_CMD);
    OLED_WriteByte(0x8D, OLED_CMD); /* 电荷泵 */
    OLED_WriteByte(0x14, OLED_CMD);
    OLED_WriteByte(0xAF, OLED_CMD); /* 开启显示 */

    OLED_Clear();
    OLED_Display();
}

/* ======================================================================== */
/*                              缓冲区操作                                   */
/* ======================================================================== */

void OLED_Clear(void)
{
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
}

void OLED_Display(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++)
    {
        OLED_SetPos(0, page);
        for (uint8_t col = 0; col < OLED_WIDTH; col++)
        {
            OLED_WriteByte(OLED_Buffer[page][col], OLED_DATA);
        }
    }
}

void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    if (color)
        OLED_Buffer[y / 8][x] |= (1 << (y % 8));
    else
        OLED_Buffer[y / 8][x] &= ~(1 << (y % 8));
}

void OLED_ClearPixel(uint8_t x, uint8_t y)
{
    OLED_SetPixel(x, y, 0);
}

/* ======================================================================== */
/*                              字符绘制                                     */
/* ======================================================================== */

/* 把 6x8 字模直接写入 buffer。x 任意,y 必须为 8 的倍数(对齐到页) */
static void OLED_PutChar6x8(uint8_t x, uint8_t page, char ch)
{
    if (ch < ' ' || ch > '~') ch = ' ';
    if (page >= OLED_PAGES) return;

    uint8_t idx = (uint8_t)(ch - ' ');
    for (uint8_t i = 0; i < 6; i++)
    {
        if (x + i >= OLED_WIDTH) break;
        OLED_Buffer[page][x + i] = Font6x8[idx][i];
    }
}

/* 把 6x8 字符放大一倍变成 12x16 显示,占两页 */
static void OLED_PutChar12x16(uint8_t x, uint8_t page, char ch)
{
    if (ch < ' ' || ch > '~') ch = ' ';
    if (page + 1 >= OLED_PAGES) return;

    uint8_t idx = (uint8_t)(ch - ' ');

    for (uint8_t i = 0; i < 6; i++)
    {
        uint8_t col = Font6x8[idx][i];

        /* 把 8 位垂直放大成 16 位:每位重复一次 */
        uint16_t expanded = 0;
        for (uint8_t b = 0; b < 8; b++)
        {
            if (col & (1 << b))
                expanded |= (3u << (b * 2));
        }
        uint8_t top    = (uint8_t)(expanded & 0xFF);
        uint8_t bottom = (uint8_t)((expanded >> 8) & 0xFF);

        /* 水平方向每列也写 2 次实现宽度放大 */
        for (uint8_t k = 0; k < 2; k++)
        {
            uint8_t xc = x + i * 2 + k;
            if (xc >= OLED_WIDTH) return;
            OLED_Buffer[page][xc]     = top;
            OLED_Buffer[page + 1][xc] = bottom;
        }
    }
}

void OLED_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    /* y 自动对齐到页(向下取整) */
    uint8_t page = y / 8;

    if (size == 16)
        OLED_PutChar12x16(x, page, ch);
    else
        OLED_PutChar6x8(x, page, ch);
}

void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t char_w = (size == 16) ? 12 : 6;

    while (*str)
    {
        if (x + char_w > OLED_WIDTH)
        {
            x = 0;
            y += (size == 16) ? 16 : 8;
        }
        if (y >= OLED_HEIGHT) break;

        OLED_DrawChar(x, y, *str, size);
        x += char_w;
        str++;
    }
}

/* 把矩形区域里的整页像素做 XOR 反转 — 用于反白显示。y 必须对齐到 8 */
void OLED_InvertPageRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t start_page = y / 8;
    uint8_t end_page   = (y + h + 7) / 8;
    if (end_page > OLED_PAGES) end_page = OLED_PAGES;

    for (uint8_t p = start_page; p < end_page; p++)
    {
        for (uint8_t i = 0; i < w; i++)
        {
            if (x + i >= OLED_WIDTH) break;
            OLED_Buffer[p][x + i] ^= 0xFF;
        }
    }
}

/* ======================================================================== */
/*                              图形                                         */
/* ======================================================================== */

void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t w)
{
    for (uint8_t i = 0; i < w; i++)
        OLED_SetPixel(x + i, y, 1);
}

void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t h)
{
    for (uint8_t i = 0; i < h; i++)
        OLED_SetPixel(x, y + i, 1);
}

void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (w == 0 || h == 0) return;
    OLED_DrawHLine(x, y, w);
    OLED_DrawHLine(x, y + h - 1, w);
    OLED_DrawVLine(x, y, h);
    OLED_DrawVLine(x + w - 1, y, h);
}

void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = 0; i < w; i++)
        for (uint8_t j = 0; j < h; j++)
            OLED_SetPixel(x + i, y + j, 1);
}

void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent)
{
    if (percent > 100) percent = 100;
    OLED_DrawRect(x, y, w, h);
    if (w > 2 && h > 2)
    {
        uint8_t fill = (uint8_t)((uint16_t)(w - 2) * percent / 100);
        if (fill > 0) OLED_FillRect(x + 1, y + 1, fill, h - 2);
    }
}

void OLED_Printf(uint8_t x, uint8_t y, uint8_t size, const char *fmt, ...)
{
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OLED_DrawString(x, y, buf, size);
}

void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1)
    {
        OLED_SetPixel(x1, y1, 1);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}
