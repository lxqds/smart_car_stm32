#ifndef ST7735S_H
#define ST7735S_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lcd.h"
#include "font.h"
#include "main.h"

#define ST7735S_DC(v)      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, (GPIO_PinState)(v))
#define ST7735S_TX(p, n)   HAL_SPI_Transmit(&hspi4, (uint8_t *)(p), (uint16_t)(n), 100)
#define ST7735S_PWM_Init()
#define ST7735S_PWM_Set(v)

extern SPI_HandleTypeDef hspi4;

// Defines are taken from
//      https://raw.githubusercontent.com/m5stack/M5StickC/master/src/utility/ST7735_Defines.h
// and are modified to fit to the M5StickC device, and are taken from
//      https://github.com/adafruit/Adafruit-ST7735-Library
#define ST7735S_INVERT_COLORS 1
#define COLSTART            26
#define ROWSTART            1

// ST7735 specific commands used in init
#define ST7735_NOP          0x00
#define ST7735_SWRESET      0x01
#define ST7735_RDDID        0x04
#define ST7735_RDDST        0x09

#define ST7735_SLPIN        0x10
#define ST7735_SLPOUT       0x11
#define ST7735_PTLON        0x12
#define ST7735_NORON        0x13

#define ST7735_INVOFF       0x20
#define ST7735_INVON        0x21
#define ST7735_DISPOFF      0x28
#define ST7735_DISPON       0x29
#define ST7735_CASET        0x2A
#define ST7735_RASET        0x2B
#define ST7735_RAMWR        0x2C
#define ST7735_RAMRD        0x2E

#define ST7735_PTLAR        0x30
#define ST7735_VSCRDEF      0x33
#define ST7735_COLMOD       0x3A
#define ST7735_MADCTL       0x36
#define ST7735_VSCRSADD     0x37

#define ST7735_FRMCTR1      0xB1
#define ST7735_FRMCTR2      0xB2
#define ST7735_FRMCTR3      0xB3
#define ST7735_INVCTR       0xB4
#define ST7735_DISSET5      0xB6

#define ST7735_PWCTR1       0xC0
#define ST7735_PWCTR2       0xC1
#define ST7735_PWCTR3       0xC2
#define ST7735_PWCTR4       0xC3
#define ST7735_PWCTR5       0xC4
#define ST7735_VMCTR1       0xC5

#define ST7735_RDID1        0xDA
#define ST7735_RDID2        0xDB
#define ST7735_RDID3        0xDC
#define ST7735_RDID4        0xDD

#define ST7735_PWCTR6       0xFC

#define ST7735_GMCTRP1      0xE0
#define ST7735_GMCTRN1      0xE1

#define ST77XX_MADCTL_MY    0x80
#define ST77XX_MADCTL_MX    0x40
#define ST77XX_MADCTL_MV    0x20
#define ST77XX_MADCTL_ML    0x10
#define ST77XX_MADCTL_RGB   0x00
#define ST77XX_MADCTL_BGR   0x08

void st7735s_init(void);
void st7735s_flush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color);
void st7735s_sleep_in(void);
void st7735s_sleep_out(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ST7735S_H*/
