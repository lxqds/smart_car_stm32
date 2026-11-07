#ifndef TEST_LOG_TIM_H
#define TEST_LOG_TIM_H
#include <stdint.h>
typedef struct {uint32_t count;} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim1;
#define TIM_CHANNEL_ALL 0xffffU
#define __HAL_TIM_GET_COUNTER(t) ((t)->count)
int HAL_TIM_Encoder_Start(TIM_HandleTypeDef *t,uint32_t channel);
#endif
