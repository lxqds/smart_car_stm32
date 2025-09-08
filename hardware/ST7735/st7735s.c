/**
 * @file st7735s.c
 *
 */
#include "st7735s.h"

static void st7735s_send_cmd(uint8_t cmd)
{
    ST7735S_DC(0);	 //Command mode
    ST7735S_TX(&cmd, 1);
    ST7735S_DC(1);	 //Data mode
}

static void st7735s_send_data(void *data, uint16_t length)
{
    ST7735S_TX(data, length);
}

void st7735s_sleep_in()
{
    st7735s_send_cmd(ST7735_SLPIN);
}

void st7735s_sleep_out()
{
    st7735s_send_cmd(ST7735_SLPOUT);
}

static void st7735s_set_orientation(uint8_t orientation)
{
    /*
        Portrait:  0xC8 = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_BGR
        Landscape: 0xA8 = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_BGR
				Portrait:  0x98 = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_BGR
        Landscape_inverted: 0x68 = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_BGR
        Remark: "inverted" is ignored here
        "PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE", "LANDSCAPE_INVERTED"
    */
    uint8_t data[] = {0xC8, 0xA8, 0x98, 0x68};

    st7735s_send_cmd(ST7735_MADCTL);
    st7735s_send_data((void *) &data[orientation], 1);
}

void LCD_Init(void)
{
    static const uint8_t init_cmds[] = {
        0x80, ST7735_SWRESET,         	      // Software reset, 0 args, w/delay 150
        0x80, ST7735_SLPOUT,                  // Out of sleep mode, 0 args, w/delay 500
        3, ST7735_FRMCTR1, 0x01, 0x2C, 0x2D,  // Frame rate ctrl - normal mode, 3 args: Rate = fosc/(1x2+40) * (LINE+2C+2D)
        3, ST7735_FRMCTR2, 0x01, 0x2C, 0x2D,  // Frame rate control - idle mode, 3 args:Rate = fosc/(1x2+40) * (LINE+2C+2D)
        6, ST7735_FRMCTR3, 0x01, 0x2C, 0x2D,0x01, 0x2C, 0x2D, //Frame rate ctrl - partial mode, 6 args:Dot inversion mode. Line inversion mode
        1, ST7735_INVCTR, 0x07,               // Display inversion ctrl, 1 arg, no delay:No inversion
        3, ST7735_PWCTR1, 0xA2,0x02, 0x84,    // Power control, 3 args, no delay:-4.6V AUTO mode
        1, ST7735_PWCTR2, 0xC5,               // Power control, 1 arg, no delay:VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
        2, ST7735_PWCTR3, 0x0A, 0x00,         // Power control, 2 args, no delay: Opamp current small, Boost frequency
        2, ST7735_PWCTR4, 0x8A, 0x2A,         // Power control, 2 args, no delay: BCLK/2, Opamp current small & Medium low
        2, ST7735_PWCTR5, 0x8A, 0xEE,         // Power control, 2 args, no delay:
        1, ST7735_VMCTR1, 0x0E,               // Power control, 1 arg, no delay:
#if ST7735S_INVERT_COLORS == 1
        0, ST7735_INVON,                      // set inverted mode
#else
        0, ST7735_INVOFF,                     // set non-inverted mode
#endif
        1, ST7735_COLMOD, 0x05,               // set color mode, 1 arg, no delay: Pixel format chosen is RGB565 : 16 bpp
        16, ST7735_GMCTRP1, 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                            0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,  // 16 args, no delay:
        16, ST7735_GMCTRN1, 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                            0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,  // 16 args, no delay:
        0x80, ST7735_NORON,    	// Normal display on, no args, w/delay 10 ms delay
        0x80, ST7735_DISPON,    // Main screen turn on, no args w/delay 100 ms delay
    };

    //Send all the commands
    uint16_t i = 0;
    while (i < sizeof(init_cmds)) {
        uint8_t databytes = init_cmds[i++];//
        uint8_t delay = databytes & 0x80;//判断数据最高位是否为1
        databytes &= 0x1F;							 //确保数据最大长度为16位，不溢出
        st7735s_send_cmd(init_cmds[i++]);
        st7735s_send_data((void *)&init_cmds[i], databytes);
        if (delay) {
            HAL_Delay(100);
        }
        i += databytes;
    }

    st7735s_set_orientation(DISP_ORIENTATION);
    ST7735S_PWM_Init();
}

void LCD_Brightness(int value)
{
    ST7735S_PWM_Set(value);
}

void LCD_DrawRaw(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color_map)
{
    uint8_t data[4];
    if (x2<x1 || y2<y1) return;

    x1 += (DISP_ORIENTATION ? ROWSTART : COLSTART);
    x2 += (DISP_ORIENTATION ? ROWSTART : COLSTART);
    y1 += (DISP_ORIENTATION ? COLSTART : ROWSTART);
    y2 += (DISP_ORIENTATION ? COLSTART : ROWSTART);

    //Column addresses
    st7735s_send_cmd(ST7735_CASET);
    data[0] = x1 >> 8;
    data[1] = x1;
    data[2] = x2 >> 8;
    data[3] = x2;
    st7735s_send_data(data, 4);

    //Page addresses
    st7735s_send_cmd(ST7735_RASET);
    data[0] = y1 >> 8;
    data[1] = y1;
    data[2] = y2 >> 8;
    data[3] = y2;
    st7735s_send_data(data, 4);

    //Memory write
    st7735s_send_cmd(ST7735_RAMWR);
    if (!color_map) return;

    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
    st7735s_send_data((void*)color_map, size * 2);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_DrawRaw(x, y, x, y, &color);
}

#define DISP_BUFSIZE 512
static uint16_t disp_buff[DISP_BUFSIZE];

/**
  * @brief  Draws a full RGB rectangle
  */
void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int i, size = (x2-x1+1) * (y2-y1+1);
    int len = (size>DISP_BUFSIZE) ? DISP_BUFSIZE : size;
    LCD_DrawRaw(x1, y1, x2, y2, 0);
    for (i=0; i<len; i++)
        disp_buff[i] = color;
    for(; size; size -= len)
    {
        st7735s_send_data(disp_buff, 2*len);
        len = (size>DISP_BUFSIZE) ? DISP_BUFSIZE : size;
    }
}

static const FontDef *char_font = &font_ascii_1608;
static uint16_t char_forecolor = 0xffff;
static uint16_t char_backcolor = 0;
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

void LCD_SetFont(const FontDef *pFont)
{
    char_font = pFont;
}

void LCD_SetFontColor(uint16_t forecolor, uint16_t backcolor)
{
    char_forecolor = forecolor;
    char_backcolor = backcolor;
}

void LCD_SetCursor(uint16_t x, uint16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void LCD_DrawChar(const char c)
{
    uint8_t w = char_font->width;  // font width
    uint8_t h = char_font->height; // font height
    if (c == '\n' || c == '\r') {
        cursor_y += h;
        cursor_x = 0;
    }
    if (c < ' ' || c > '~') return;
    if (cursor_x >= DISP_WIDTH || cursor_x+w > DISP_WIDTH || cursor_y >= DISP_HEIGHT || cursor_y+h > DISP_HEIGHT) return;

    uint8_t s = (h+7) / 8 * w;     // font bytes
    uint8_t *p = char_font->data + (c - ' ') * s;
    uint8_t x,y,temp;
    for(x=0; x<w; x++)
    {
        for(y=0; y<h; y++)
        {
            if ((y % 8) == 0) temp = *p++;
            //LCD_DrawPixel(cursor_x + x, cursor_y + y, (temp&0x80)? char_forecolor: char_backcolor);
            disp_buff[y*w + x] = (temp&0x80)? char_forecolor: char_backcolor;
            temp <<= 1;
        }
    }
    LCD_DrawRaw(cursor_x, cursor_y, cursor_x+w-1, cursor_y+h-1, disp_buff);
    cursor_x += w;
}   

void LCD_DrawString(const char *p)
{
    char c;
    while ((c = *p++))
    {       
        LCD_DrawChar(c);
    }
}

void LCD_show_string(uint8_t x,uint8_t y,const char *p)
{
	LCD_SetCursor(x, y);
  LCD_SetFont(&font_ascii_1206);
  LCD_DrawString(p);
}

void LCD_Test(void)
{
    LCD_FillRect(0,0,159,79,0);

    LCD_FillRect(102,0,119,5,SWAP(RED));
    LCD_FillRect(122,0,139,5,SWAP(GREEN));
    LCD_FillRect(142,0,159,5,SWAP(BLUE));

    LCD_SetCursor(0, 0);
    LCD_SetFont(&font_ascii_2412);
    LCD_DrawString("ABCDEFGH\n1234567890");
}
/*************************************************
函数名：Lcd_Clear
功能：全屏清屏函数
入口参数：填充颜色COLOR
返回值：无
*************************************************/
void LCD_Clear()               
{	
   LCD_FillRect(0,0,DISP_WIDTH,DISP_HEIGHT,SWAP(BLACK));
}
/*************************************************
函数名：Lcd_Clear
功能：全屏清屏函数
入口参数：填充颜色COLOR
返回值：无
*************************************************/
void LCD_Clear2(uint16_t color)               
{	
   LCD_FillRect(0,0,DISP_WIDTH,DISP_HEIGHT,color);
}
/**
 * @name   LCD_DrawLine
 * @brief  LCD使用算法画直线
 * @param  usX_Start：X轴起始坐标
 * 			usY_Start：Y轴起始坐标
 * 			usX_End：X轴终点坐标
 * 			usY_End：Y轴终点坐标
 * 			usColor：显示的颜色
 * @retval None  
 */
void LCD_DrawLine(uint16_t usX_Start,uint16_t usY_Start,uint16_t usX_End,uint16_t usY_End,uint16_t usColor)
{
	uint16_t i;
	uint16_t j;					//动态查看画线效果变量
	uint16_t usX_Current,usY_Current;
	int16_t sError_X=0,sError_Y=0,sDelta_X,sDelta_Y,sDistance; 
	int16_t sIncrease_X, sIncrease_Y;

	//设置坐标增量
	sDelta_X = usX_End - usX_Start;
	sDelta_Y = usY_End - usY_Start;
	usX_Current = usX_Start;
	usY_Current = usY_Start;

	//算法，直接拿来用
	if(sDelta_X > 0)
  	{
    	/* 设置单步方向 */
    	sIncrease_X=1; 
	}
  	else if(sDelta_X == 0)
  	{
    	/* 垂直线 */
		sIncrease_X = 0;
	}
	else 
  	{ 
   		sIncrease_X = -1;
    	sDelta_X    = -sDelta_X;
  	}
  
	if(sDelta_Y > 0)
  	{
		sIncrease_Y = 1;
	}
	else if(sDelta_Y==0)
  	{
    	/* 水平线  */
		sIncrease_Y = 0;
	}
	else
  	{
    	sIncrease_Y = -1;
    	sDelta_Y    = -sDelta_Y;
  	}
	
	if(sDelta_X > sDelta_Y)
  	{
    	/* 选取基本增量坐标轴 */
		sDistance = sDelta_X;  
	}
	else
  	{
		sDistance = sDelta_Y; 
  	}

	//画线输出
	for(i=0;i<=sDistance+1;i++) 
	{
    	//画点
		LCD_DrawPixel(usX_Current, usY_Current,usColor);
		sError_X += sDelta_X; 
		sError_Y += sDelta_Y;
		if(sError_X > sDistance)
		{ 
			sError_X    -= sDistance; 
			usX_Current += sIncrease_X; 
		}
		if(sError_Y > sDistance) 
		{ 
			sError_Y    -= sDistance; 
			usY_Current += sIncrease_Y; 
		}	
		
		//动态看画线效果，可删除
		for(j=0;j<20000;j++);
	}
}

void LCD_DrawRect2(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
		LCD_DrawLine(x1,y1,x1,y2,color);//竖
		LCD_DrawLine(x1,y1,x2,y1,color);//横
		LCD_DrawLine(x2,y1,x2,y2,color);//竖
		LCD_DrawLine(x1,y2,x2,y2,color);//横
}
void LCD_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,uint16_t color)
{
		LCD_DrawLine(x1,y1,x2,y2,color);//斜边
		LCD_DrawLine(x1,y1,x3,y3,color);//斜边
		LCD_DrawLine(x2,y2,x3,y3,color);//横边
}

//画圆
void LCD_DrawCircle2(uint8_t Xpos, uint8_t Ypos, uint16_t Radius,uint16_t color)
{
	int32_t  D;
	uint32_t  CurX;
	uint32_t  CurY;
  
  	D = 3 - (Radius << 1);
  	CurX = 0;
  	CurY = Radius;
  
  	while (CurX <= CurY)
  	{
	    LCD_DrawPixel(Xpos + CurX, Ypos + CurY,color);

	    LCD_DrawPixel(Xpos + CurX, Ypos - CurY,color);
	
	    LCD_DrawPixel(Xpos - CurX, Ypos + CurY,color);
	
	    LCD_DrawPixel(Xpos - CurX, Ypos - CurY,color);
	
	    LCD_DrawPixel(Xpos + CurY, Ypos + CurX,color);

	    LCD_DrawPixel(Xpos + CurY, Ypos - CurX,color);
	
	    LCD_DrawPixel(Xpos - CurY, Ypos + CurX,color);
	
	    LCD_DrawPixel(Xpos - CurY, Ypos - CurX,color);

	    if (D < 0)
	    { 
	      D += (CurX << 2) + 6;
	    }
	    else
	    {
	      D += ((CurX - CurY) << 2) + 10;
	      CurY--;
	    }
	    CurX++;
		}

}
