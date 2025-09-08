#include "key.h"
uint8_t B1[5];
uint8_t release[5];
uint16_t down_count[5];
uint8_t last_B1[5];
void key_scan(uint8_t *key_value)
{
	B1[0] = HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_12);
	B1[1] = HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_13);
	B1[2] = HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_14);
	B1[3] = HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_15);
	B1[4] = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_8);
	for(uint8_t i=0;i<5;i++)
	{
		if(B1[i]==0&&last_B1[i]==0)
		{
			release[i] = 0;
			down_count[i] ++;
		}else if(B1[i]==1&&last_B1[i]==1)
		{
			release[i] = 1;
			
		}
		last_B1[i] = B1[i];

		if(release[i])
		{
			if(down_count[i]>2&&down_count[i]<200)
			{
				down_count[i]=0;
				*key_value = 11+i*10;
			}else if(down_count[i]>200)
			{
				down_count[i]=0;
				*key_value = 12+i*10;
			}
		}
	}
}