#include "timer.h"
/*
********************************************************************************************************
*                                               示例代码
*                                             EXAMPLE  CODE                                             
*
*                             (c) Copyright 2021; SaiShu.Lcc.; Leo
*                                 版权所属[北京赛曙科技有限公司]
*
*               The code is for internal use only, not for commercial transactions(开源学习,请勿商用).
*               The code ADAPTS the corresponding hardware circuit board(代码使用CarDo智控板), 
*               the specific details consult the professional(欢迎联系我们).
*********************************************************************************************************
*/


/**
* @brief        定时器TIM2初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void TIM2_Init(void)
{
	pit_ms_init(PIT_CH0,10);//10ms
	pit_enable(PIT_CH0);
	pit_ms_init(PIT_CH1,1);//1ms
	pit_enable(PIT_CH1);
	pit_ms_init(PIT_CH2,10);//10ms
	pit_enable(PIT_CH2);
}













