#include "servo.h"
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

ServoStruct servoStr;


/**
* @brief        舵机控制初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void SERVO_Init(void)
{
//    //PWM-IO初始化
//    GPIO_InitTypeDef GPIO_InitStructure;
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  
//    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6; 			//PWM
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
//    GPIO_Init(GPIOB, &GPIO_InitStructure);
//	
//    //TIM初始化
//    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
//    TIM_OCInitTypeDef TIM_OCInitStructure;  
//    TIM_BDTRInitTypeDef TIM_BDTRInitStructure; 
//	
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
//    TIM_TimeBaseStructure.TIM_Prescaler = 72-1;  												// 系统  72MHz    TIM时钟= 72MHz/72=1M			
//    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
//    TIM_TimeBaseStructure.TIM_Period = 20000-1;   											// Frequency = 1000000 / 20000 = 50Hz
//    TIM_TimeBaseStructure.TIM_ClockDivision = 0;   
//    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     
//    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);    
//      
//    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 									//设置PWM模式为向上计数模式 
//    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  
//    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;  
//    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  
//    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;  
//    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;  
//    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;  
//    TIM_OCInitStructure.TIM_Pulse = 0;  

//    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
//    TIM_OC1Init(TIM4, &TIM_OCInitStructure);  

//    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  
//    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;  
//    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  
//    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;  
//    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;  
//    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;  
//    TIM_OCInitStructure.TIM_Pulse = 0;  
//		
//    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable; // Disable the Break function  
//    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;  
//    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable; //Enable Running State  
//    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable; //Enable Idle State  
//    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF; //Set the lock level  
//    TIM_BDTRInitStructure.TIM_DeadTime = 0x2B;  
//    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable; //Enable the Auto Outputting.  
//    TIM_BDTRConfig(TIM4, &TIM_BDTRInitStructure);  
//      
//    TIM_Cmd(TIM4, ENABLE);    
//    TIM_CtrlPWMOutputs(TIM4, ENABLE);   
	pwm_init(PWM4_MODULE2_CHA_C30,50,1500);   
	servoStr.thresholdMiddle =1500;
	servoStr.thresholdLeft =1140;
	servoStr.thresholdRight =2000;
    SERVO_SetPwmValue(servoStr.thresholdMiddle);
}


/**
* @brief        舵机输出PWM设置
* @param        pwm：-20000~20000
* @ref          
* @author       Leo
* @note         
**/
void SERVO_SetPwmValue(signed int pwm)
{   
    pwm = 3000 - pwm;  //左→右
        
    if(pwm < SERVO_PWM_MIN)
        pwm = SERVO_PWM_MIN;
    else if(pwm > SERVO_PWM_MAX)
        pwm = SERVO_PWM_MAX;
 
    pwm_set_duty(PWM4_MODULE2_CHA_C30,pwm);
}


/**
* @brief        舵机输出PWM设置（矫正后）
* @param        pwm：500~2500
* @ref          
* @author       Leo
* @note         
**/
uint16_t pwm_Servo = 0;
void SERVO_SetPwmValueCorrect(signed int pwm)
{   
    pwm = 3000 - pwm;  //左→右
    
    pwm -= servoStr.thresholdMiddle-SERVO_PWM_MIDDLE; //中值补偿
	
	uint16_t pwmMax = 3000 - servoStr.thresholdLeft;
	uint16_t pwmMin = 3000 - servoStr.thresholdRight;
	if(pwm < pwmMin)
        pwm = pwmMin;
    else if(pwm > pwmMax)
        pwm = pwmMax;
	
    pwm_set_duty(PWM4_MODULE2_CHA_C30,pwm);
}

/**
* @brief       舵机角度控制 
* @param        
* @ref          
* @author       
* @note         
**/
uint16_t ServoPwm = 1560;
void SERVO_AngleControl(float angle)
{
	uint16_t pwm = 1560;
	angle = -angle;
	if(angle > SERVO_ANGLE_MAX)
		angle = SERVO_ANGLE_MAX;
	else if(angle < -SERVO_ANGLE_MAX)
		angle = -SERVO_ANGLE_MAX;
	
	if(angle >= 0)  //右转
		pwm = (float)angle/SERVO_ANGLE_MAX * (SERVO_PWM_MAX_R-servoStr.thresholdMiddle) + servoStr.thresholdMiddle;		//绝对角度计算
	else if(angle < 0)  	//左转
		pwm = (float)angle/SERVO_ANGLE_MAX * (servoStr.thresholdMiddle - SERVO_PWM_MAX_L) + servoStr.thresholdMiddle;		//绝对角度计算
	
	ServoPwm = pwm;
	SERVO_SetPwmValue(pwm);
}

