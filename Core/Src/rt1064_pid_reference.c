/* RT1064 0a878262 portable arithmetic reference; original/license in reference/.
 * Not selected by VehicleCore. Retains legacy reset/saturation semantics. */
#include "rt1064_pid_reference.h"
#include "math.h"



RT1064_PID RT1064_speed_pid;
float RT1064_current_speed, RT1064_target_speed;


void RT1064_PID_Limit(float* data, float max, float min)
{
    if (*data > max) {
        *data = max;
    }
    else if (*data < min) {
        *data = min;
    }
}

void RT1064_Pid_Reset(RT1064_PID* pid)
{
    pid->Pout = 0;
    pid->Iout = 0;
    pid->Dout = 0;

    pid->Out = 0;
}

void RT1064_Pid_Reset_I(RT1064_PID* pid)
{
    pid->Iout = 0;

}


float RT1064_Pid_Calculate(RT1064_PID* pid, float Target, float Current)
{
    float out = 0;

    if (Target - Current > pid->err_limit)pid->err[RT1064_Now] = pid->err_limit;
    else if (Target - Current < -pid->err_limit)pid->err[RT1064_Now] = -pid->err_limit;
    else pid->err[RT1064_Now] = Target - Current;  


    if (pid->PID_Mode == RT1064_Position_PID)  
    {

        pid->Pout = pid->Kp * pid->err[RT1064_Now];  
        if (pid->Out < pid->Max_Out && pid->Out > pid->Min_Out)
            pid->Iout += pid->Ki * pid->err[RT1064_Now];  
        RT1064_PID_Limit((&pid->Iout), pid->Integral_limit, -pid->Integral_limit);  
        pid->Dout = pid->Kd * (pid->err[RT1064_Now] - pid->err[RT1064_Last]);  

        pid->Out = pid->Pout + pid->Iout + pid->Dout;  
        out = pid->Out;
        RT1064_PID_Limit(&out, pid->Max_Out, pid->Min_Out);  

    }
    else if (pid->PID_Mode == RT1064_Delta_PID)  
    {
        pid->Pout = pid->Kp * (pid->err[RT1064_Now] - pid->err[RT1064_Last]);  
        pid->Iout = pid->Ki * pid->err[RT1064_Now];  
        pid->Dout = pid->Kd * (pid->err[RT1064_Now] - 2 * pid->err[RT1064_Last] + pid->err[RT1064_LLast]);  
        RT1064_PID_Limit((&pid->Iout), pid->Integral_limit, pid->Min_Out);  
        pid->Out += pid->Pout + pid->Dout;  
        if (pid->Iout > 0 && pid->Out + pid->Iout < pid->Max_Out) {
            pid->Out += pid->Iout;
        }
        else if (pid->Iout < 0 && pid->Out + pid->Iout > pid->Min_Out) {
            pid->Out += pid->Iout;
        }
        out = pid->Out;
        RT1064_PID_Limit(&out, pid->Max_Out, pid->Min_Out);  
    }
    pid->err[RT1064_LLast] = pid->err[RT1064_Last];  
    pid->err[RT1064_Last] = pid->err[RT1064_Now];

    return out;  
}

void RT1064_PID_Init(
    RT1064_PID* pid,
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

void RT1064_PID_Set(
    RT1064_PID* pid,
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

