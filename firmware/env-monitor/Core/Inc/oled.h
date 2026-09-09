#ifndef __OLED_H
#define __OLED_H

#include "stm32l1xx_hal.h"
#include <stdint.h>

// OLED引脚定义 (SPI)
#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_PIN_12
#define OLED_DC_PORT    GPIOB
#define OLED_DC_PIN     GPIO_PIN_14
#define OLED_SCK_PORT   GPIOB
#define OLED_SCK_PIN    GPIO_PIN_13
#define OLED_SDA_PORT   GPIOB
#define OLED_SDA_PIN    GPIO_PIN_15

// 屏幕尺寸
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      8

// 列偏移(某些 SSD1306 模块有效区从第 2 列开始,左侧会被切掉)
#define OLED_X_OFFSET   2

// 命令/数据
#define OLED_CMD        0
#define OLED_DATA       1

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display(void);
void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_ClearPixel(uint8_t x, uint8_t y);
void OLED_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t size);
void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t w);
void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t h);
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent);
void OLED_Printf(uint8_t x, uint8_t y, uint8_t size, const char *fmt, ...);
void OLED_InvertPageRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

#endif
