#ifndef TEST_PORT_MAIN_H
#define TEST_PORT_MAIN_H
#include <stdint.h>
#define GPIOA 0
#define GPIO_PIN_0 1
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define HAL_OK 0
void HAL_GPIO_WritePin(int port,int pin,int value);
#endif
