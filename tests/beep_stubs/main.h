#ifndef TEST_BEEP_MAIN_H
#define TEST_BEEP_MAIN_H
#include <stdint.h>
#define BEEP_GPIO_Port 3
#define BEEP_Pin (1U<<11)
#define GPIO_PIN_RESET 0
#define HAL_OK 0
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(v) ((void)(v))
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(int port,unsigned pin,int level);
void HAL_GPIO_TogglePin(int port,unsigned pin);
#endif
