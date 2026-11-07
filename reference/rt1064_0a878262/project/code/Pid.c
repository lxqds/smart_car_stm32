#include "pid.h"
#include "math.h"



PID speed_pid;
float current_speed, target_speed;


void PID_Limit(float* data, float max, float min)
{
    if (*data > max) {
        *data = max;
    }
    else if (*data < min) {
        *data = min;
    }
}

void Pid_Reset(PID* pid)
{
    pid->Pout = 0;
    pid->Iout = 0;
    pid->Dout = 0;

    pid->Out = 0;
}

void Pid_Reset_I(PID* pid)
{
    pid->Iout = 0;

}


float Pid_Calculate(PID* pid, float Target, float Current)
{
    float out = 0;

    if (Target - Current > pid->err_limit)pid->err[Now] = pid->err_limit;
    else if (Target - Current < -pid->err_limit)pid->err[Now] = -pid->err_limit;
    else pid->err[Now] = Target - Current;  // 当前误差为目标值与当前值之差


    if (pid->PID_Mode == Position_PID)  // 如果是位置型PID控制
    {

        pid->Pout = pid->Kp * pid->err[Now];  // 比例项输出
        if (pid->Out < pid->Max_Out && pid->Out > pid->Min_Out)//若没有超界
            pid->Iout += pid->Ki * pid->err[Now];  // 积分项输出
        PID_Limit((&pid->Iout), pid->Integral_limit, -pid->Integral_limit);  // 对积分项进行限幅
        pid->Dout = pid->Kd * (pid->err[Now] - pid->err[Last]);  // 微分项输出

        pid->Out = pid->Pout + pid->Iout + pid->Dout;  // PID控制器输出
        out = pid->Out;
        PID_Limit(&out, pid->Max_Out, pid->Min_Out);  // 对PID输出进行限幅

    }
    else if (pid->PID_Mode == Delta_PID)  // 如果是增量型PID控制
    {
        pid->Pout = pid->Kp * (pid->err[Now] - pid->err[Last]);  // 比例项输出
        pid->Iout = pid->Ki * pid->err[Now];  // 积分项输出
        pid->Dout = pid->Kd * (pid->err[Now] - 2 * pid->err[Last] + pid->err[LLast]);  // 微分项输出
        PID_Limit((&pid->Iout), pid->Integral_limit, pid->Min_Out);  // 对积分项进行限幅
        pid->Out += pid->Pout + pid->Dout;  // PID控制器输出
        if (pid->Iout > 0 && pid->Out + pid->Iout < pid->Max_Out) {
            pid->Out += pid->Iout;
        }
        else if (pid->Iout < 0 && pid->Out + pid->Iout > pid->Min_Out) {
            pid->Out += pid->Iout;
        }
        out = pid->Out;
        PID_Limit(&out, pid->Max_Out, pid->Min_Out);  // 对PID输出进行限幅
    }
    pid->err[LLast] = pid->err[Last];  // 更新误差历史数据
    pid->err[Last] = pid->err[Now];

    return out;  // 返回PID控制器的输出值
}

void PID_Init(
    PID* pid,
    uint8_t PID_Mode,
    float Max_Out,
    float Min_Out,
    float err_limit,
    float Integral_limit,
    float Kp,
    float Ki,
    float Kd)
{
    pid->PID_Mode = PID_Mode;
    pid->Max_Out = Max_Out;
    pid->Min_Out = Min_Out;
    pid->err_limit = err_limit;
    pid->Integral_limit = Integral_limit;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;

}

void PID_Set(
    PID* pid,
    float err_limit,
    float Integral_limit,
    float Kp,
    float Ki,
    float Kd)
{
    pid->err_limit = err_limit;
    pid->Integral_limit = Integral_limit;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;

}

