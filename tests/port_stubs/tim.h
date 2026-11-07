#ifndef TEST_PORT_TIM_H
#define TEST_PORT_TIM_H
#include <stdint.h>
typedef struct {uint32_t EGR;} TestTim;
typedef struct {TestTim *Instance;uint32_t psc,arr,ccr;} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim2,htim5;
#define TIM_CHANNEL_2 2
#define TIM_CHANNEL_3 3
#define TIM_EGR_UG 1
#define __HAL_TIM_SET_PRESCALER(t,v) ((t)->psc=(v))
#define __HAL_TIM_SET_AUTORELOAD(t,v) ((t)->arr=(v))
#define __HAL_TIM_GET_COMPARE(t,c) ((t)->ccr)
#define __HAL_TIM_GET_AUTORELOAD(t) ((t)->arr)
#define __HAL_TIM_SET_COMPARE(t,c,v) ((t)->ccr=(v))
void MX_TIM2_Init(void);
void MX_TIM5_Init(void);
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *t,int channel);
int HAL_TIM_PWM_Stop(TIM_HandleTypeDef *t,int channel);
#endif
