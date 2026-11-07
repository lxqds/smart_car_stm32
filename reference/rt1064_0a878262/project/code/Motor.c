#include "motor.h"
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

MotorStruct motorStr;
extern float current_speed, target_speed;
extern PID speed_pid;

/**
* @brief        电机控制初始化
* @param
* @ref
* @author       Leo
* @note
**/
void MOTOR_Init(void)
{

    gpio_init(D1, GPO, 0, GPO_PUSH_PULL);
    pwm_init(PWM1_MODULE3_CHA_D0, 5000, 0);

    MOTOR_SetPwmValue(0);

    //电机模型初始化
    motorStr.EncoderLine = 512.0f; 							//编码器线数=光栅数16*4				
    motorStr.ReductionRatio = 2.7f;							//电机减速比								
    motorStr.EncoderValue = 0;
    motorStr.DiameterWheel = 0.064f;//68cm					//轮子直径:m
    motorStr.CloseLoop = true;                              //默认闭环模式
}


/**
* @brief        电机输出PWM设置
* @param        pwm：-2000~2000
* @ref
* @author       Leo
* @note
**/
void MOTOR_SetPwmValue(signed int pwm)
{
    pwm = -pwm;
    if (pwm > 0) {
        gpio_set_level(D1, 0);
        if (pwm > MOTOR_PWM_MAX)
            pwm = MOTOR_PWM_MAX;

        pwm_set_duty(PWM1_MODULE3_CHA_D0, abs(pwm) * 10);

    }
    else if (pwm < 0) {
        gpio_set_level(D1, 1);
        if (pwm < MOTOR_PWM_MIN)
            pwm = MOTOR_PWM_MIN;
        pwm = -pwm;
        pwm_set_duty(PWM1_MODULE3_CHA_D0, abs(pwm) * 10);
    }
    else {
        pwm_set_duty(PWM1_MODULE3_CHA_D0, 0);
    }


}


/**
* @brief        电机闭环速控
* @param        speed：速度m/s
* @ref
* @author       Leo
* @note
**/
void MOTOR_ControlLoop(float speed)
{
    if (speed > MOTOR_SPEED_MAX)
        speed = MOTOR_SPEED_MAX;
    else if (speed < -MOTOR_SPEED_MAX)
        speed = -MOTOR_SPEED_MAX;

    //返还编码器实时速度
// pidStr.vi_Ref = (float)(speed*MOTOR_CONTROL_CYCLE / motorStr.DiameterWheel / PI * motorStr.EncoderLine * motorStr.ReductionRatio);
    target_speed = (float)(speed * MOTOR_CONTROL_CYCLE / motorStr.DiameterWheel / PI * motorStr.EncoderLine * motorStr.ReductionRatio);
    // MOTOR_SetPwmValue(PID_MoveCalculate(&pidStr));
    MOTOR_SetPwmValue(Pid_Calculate(&speed_pid, target_speed, current_speed));
}


/**
* @brief        电机控制线程
* @param
* @ref
* @author       Leo
* @note
**/
void MOTOR_Timer(void)
{
    motorStr.Counter++;
    if (motorStr.Counter >= 10)							    //速控:10ms
    {
        ENCODER_RevSample();								//编码器采样

        if (icarStr.sprintEnable || usbStr.connected)        //通信连接或电机测试才开启闭环（保护+省电）
        {
            if (motorStr.CloseLoop) {
                MOTOR_ControlLoop(icarStr.SpeedSet);		//闭环速控
            }

            else//开环百分比控制
            {
                if (icarStr.SpeedSet > 100)
                    icarStr.SpeedSet = 100;
                else if (icarStr.SpeedSet < -100)
                    icarStr.SpeedSet = -100;
                signed int speedRate = MOTOR_PWM_MAX / 100.f * icarStr.SpeedSet; //开环：百分比%

                MOTOR_SetPwmValue(speedRate);		//开环速控
            }
        }
        else {
            Pid_Reset(&speed_pid);
            icarStr.SpeedSet = 0.0f;
            MOTOR_SetPwmValue(0);
        }

        motorStr.Counter = 0;
    }
}

