#ifndef ST7735_H
#define ST7735_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* RGB565 colours */
#define BLACK     0x0000
#define WHITE     0xFFFF
#define RED       0xF800
#define GREEN     0x07E0
#define BLUE      0x001F
#define CYAN      0x07FF
#define YELLOW    0xFFE0
#define ORANGE    0xFD20
#define GRAY      0x8410
#define DARKGRAY  0x4208
#define NAVY      0x000F
#define MAGENTA   0xF81F
#define LIME      0x07E0
#define MAROON    0x7800
#define PURPLE    0x780F
#define TEAL      0x0410

/* LCD dimensions */
#define LCD_WIDTH    80
#define LCD_HEIGHT   160
#define LCD_X_OFFSET 26
#define LCD_Y_OFFSET  1

/* Public API */
void ST7735_Init(SPI_HandleTypeDef *hspi);
void ST7735_FillScreen(uint16_t color);
void ST7735_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void ST7735_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color, uint8_t thickness);
void ST7735_DrawPixel(uint8_t x, uint8_t y, uint16_t color);
void ST7735_DrawChar(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void ST7735_DrawString(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);
void ST7735_DrawHLine(uint8_t x, uint8_t y, uint8_t len, uint16_t color);
void ST7735_DrawVLine(uint8_t x, uint8_t y, uint8_t len, uint16_t color);

#endif