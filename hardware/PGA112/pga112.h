#ifndef __PGA112_H
#define __PGA112_H
#include "main.h"
#include "gpio.h"

//  HAL_GPIO_WritePin(GPIOE, PGA_SCLK_A_Pin|PGA_DIO_A_Pin|PGA_CS_A_Pin, GPIO_PIN_SET);

//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOD, PGA_SCLK_B_Pin|PGA_DIO_B_Pin|PGA_CS_B_Pin, GPIO_PIN_SET);



//CS片选低电平为有效使能， CS片选高电平不使能
#define PGA_A_CS_Enable()					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET)
#define PGA_A_CS_Disable()				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET)

//#define PGA_B_CS_Enable()					HAL_GPIO_WritePin(GPIOD,PGA_CS_B_Pin,GPIO_PIN_RESET)
//#define PGA_B_CS_Disable()				HAL_GPIO_WritePin(GPIOD,PGA_CS_B_Pin,GPIO_PIN_SET)

#define PGA_A_SCLK_LOW()					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET)
#define PGA_A_SCLK_HIGH()					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET)
#define PGA_A_DIO_LOW()						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET)
#define PGA_A_DIO_HIGH()					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET)
//						 
//#define PGA_B_SCLK_LOW()					HAL_GPIO_WritePin(GPIOD,PGA_SCLK_B_Pin,GPIO_PIN_RESET)
//#define PGA_B_SCLK_HIGH()					HAL_GPIO_WritePin(GPIOD,PGA_SCLK_B_Pin,GPIO_PIN_SET)
//#define PGA_B_DIO_LOW()						HAL_GPIO_WritePin(GPIOD,PGA_DIO_B_Pin,GPIO_PIN_RESET)
//#define PGA_B_DIO_HIGH()					HAL_GPIO_WritePin(GPIOD,PGA_DIO_B_Pin,GPIO_PIN_SET)
//分别对应系统放大:

//	  gain取值：1, 2, 4,8,16,32,64,128		 

void SetGain(uint8_t gain,uint8_t pga);

#endif





