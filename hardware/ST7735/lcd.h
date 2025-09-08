#ifndef __LCD_H__
#define __LCD_H__

#define DISP_WIDTH       160
#define DISP_HEIGHT      80
#define DISP_ORIENTATION 1                                                                                                                         

#define WHITE           0xFFFF
#define BLACK           0x0000	  
#define BLUE            0x001F  
#define BRED            0XF81F
#define GRED            0XFFE0
#define GBLUE           0X07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define GREEN           0x07E0
#define CYAN            0x7FFF
#define YELLOW          0xFFE0
#define BROWN           0XBC40 //棕色
#define BRRED           0XFC07 //棕红色
#define GRAY            0X8430 //灰色

#define DARKBLUE        0X01CF	//深蓝色
#define LIGHTBLUE       0X7D7C	//浅蓝色  
#define GRAYBLUE        0X5458 //灰蓝色
#define LIGHTGREEN      0X841F //浅绿色
#define LIGHTGRAY       0XC618 //浅灰色(PANNEL),窗体背景色

#include <stdint.h>
#define SWAP(x)         ((uint16_t)( ((uint16_t)(x)<<8) | ((uint16_t)(x)>>8) ))

void LCD_Init(void);
void LCD_On(void);
void LCD_Off(void);
void LCD_Brightness(int value);
void LCD_DrawRaw(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color);
void LCD_DrawBmp(uint16_t Xpos, uint16_t Ypos, uint8_t *pBmp);

void LCD_DrawPixel(uint16_t x, uint16_t y,uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);  // fill area with the color
void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void LCD_DrawCircle2(uint8_t Xpos, uint8_t Ypos, uint16_t Radius,uint16_t color);
void LCD_DrawRect2(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,uint16_t color);
void LCD_Clear();
void LCD_Clear2(uint16_t color);
void LCD_show_string(uint8_t x,uint8_t y,const char *p);
typedef struct {
    uint8_t width;    /*!< Font width in pixels */
    uint8_t height;   /*!< Font height in pixels */
    uint8_t *data;    /*!< Pointer to data font data array */
} FontDef;

extern const FontDef font_ascii_1206;
extern const FontDef font_ascii_1608;
extern const FontDef font_ascii_2412;
extern const FontDef font_ascii_3216;

void LCD_SetFont(const FontDef *pFont);
void LCD_SetFontColor(uint16_t forecolor, uint16_t backcolor);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_DrawChar(const char ch);
void LCD_DrawString(const char *p);

void LCD_Test(void);
#endif
