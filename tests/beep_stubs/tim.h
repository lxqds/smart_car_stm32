#ifndef TEST_BEEP_TIM_H
#define TEST_BEEP_TIM_H
#include <stdint.h>
typedef struct {uint32_t EGR;} TestTim;
typedef struct {TestTim *Instance;uint32_t arr,cnt,flag;} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim13;
#define TIM_EGR_UG 1U
#define TIM_FLAG_UPDATE 1U
#define __HAL_TIM_CLEAR_FLAG(t,f) ((t)->flag=0)
#define __HAL_TIM_SET_AUTORELOAD(t,a) ((t)->arr=(a))
#define __HAL_TIM_SET_COUNTER(t,c) ((t)->cnt=(c))
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *t);
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *t);
#endif
