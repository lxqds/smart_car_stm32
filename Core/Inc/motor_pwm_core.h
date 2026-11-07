#ifndef MOTOR_PWM_CORE_H
#define MOTOR_PWM_CORE_H
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "motor_pwm.h"
#include "encoder_units.h"
#include "rt1064_pid_reference.h"
enum { MOTOR_STOP_BOOT=0, MOTOR_STOP_POWER=1, MOTOR_STOP_LINK=2,
       MOTOR_STOP_Y=3, MOTOR_STOP_RANGE=4, /* 5 reserved: retired run deadline */
       MOTOR_STOP_TIMING=6, MOTOR_STOP_FAULT=7, MOTOR_STOP_PORT=8,
       MOTOR_STOP_ENCODER=9, MOTOR_STOP_NO_MOTION=10, MOTOR_STOP_DIRECTION=11 };
typedef struct {
    uint32_t power_at,reason,last_motion;
    uint8_t power,armed,running,enabled_seen,feedback_valid;
    int16_t duty;
    int8_t drive_sign;
    uint8_t stationary_ticks;
    int32_t delta;
    float target_mps,measured_mps;
    RT1064_PID pid;
} MotorPWMCore;
static inline void MotorCore_ResetPID(MotorPWMCore *m)
{
    /* Legacy reset leaves error history behind; clear ALL state on stop/rearm. */
    memset(&m->pid,0,sizeof(m->pid));
    RT1064_PID_Init(&m->pid,RT1064_Position_PID,1950,-1950,10000,4000,
                   MOTOR_SPEED_KP,MOTOR_SPEED_KI,MOTOR_SPEED_KD);
    m->duty=0;m->target_mps=0;
}
static inline void MotorCore_Stop(MotorPWMCore *m,uint32_t reason)
{
    m->armed=0;m->running=0;m->enabled_seen=0;m->reason=reason;
    m->drive_sign=1;m->stationary_ticks=0;
    MotorCore_ResetPID(m);
}
static inline void MotorCore_Feedback(MotorPWMCore *m,int32_t delta,int valid)
{
    m->delta=delta;
    m->feedback_valid=(uint8_t)(valid && delta>-4096 && delta<4096);
    m->measured_mps=m->feedback_valid?Encoder_EstSpeedMps(delta,10):0;
}
static inline void MotorCore_Power(MotorPWMCore *m,int on,uint32_t now)
{
    m->power=(uint8_t)(on!=0);m->power_at=now;
    if(!on)MotorCore_Stop(m,MOTOR_STOP_POWER);
}
static inline int MotorCore_Arm(MotorPWMCore *m,uint32_t now,int fresh,int y,float speed)
{
    if(m->armed || !m->power || (uint32_t)(now-m->power_at)>=50U ||
       !m->feedback_valid || !fresh || y || !isfinite(speed) || speed!=0)return 0;
    MotorCore_ResetPID(m);
    m->armed=1;m->running=0;m->enabled_seen=0;m->reason=0;m->last_motion=now;
    m->drive_sign=1;m->stationary_ticks=0;
    return 1;
}
static inline void MotorCore_Step(MotorPWMCore *m,uint32_t now,uint32_t elapsed,
                                 int fresh,int y,float speed,int fault)
{
    if(!m->armed){m->duty=0;return;}
    if(!m->power || (uint32_t)(now-m->power_at)>=50U){MotorCore_Stop(m,MOTOR_STOP_POWER);return;}
    if(elapsed!=10){MotorCore_Stop(m,MOTOR_STOP_TIMING);return;}
    if(fault){MotorCore_Stop(m,MOTOR_STOP_FAULT);return;}
    if(!fresh){MotorCore_Stop(m,MOTOR_STOP_LINK);return;}
    if(!m->feedback_valid){MotorCore_Stop(m,MOTOR_STOP_ENCODER);return;}
    if(!isfinite(speed) || speed< -0.5f || speed>5){MotorCore_Stop(m,MOTOR_STOP_RANGE);return;}
    /* Require 300ms of zero target and <=4 x4 counts/10ms before direction
     * change. This is an engineering standstill threshold, not a brake. */
    if(speed==0 && m->delta>=-4 && m->delta<=4) {
        if(m->stationary_ticks<30)m->stationary_ticks++;
    } else if(speed==0 || m->delta< -4 || m->delta>4)m->stationary_ticks=0;
    if(!y){if(m->enabled_seen)MotorCore_Stop(m,MOTOR_STOP_Y);else MotorCore_ResetPID(m);return;}
    m->enabled_seen=1;
    if(speed==0){MotorCore_ResetPID(m);m->running=0;m->last_motion=now;return;}
    int sign=speed<0?-1:1;
    if(sign!=m->drive_sign) {
        if(m->duty || m->stationary_ticks<30){MotorCore_Stop(m,MOTOR_STOP_DIRECTION);return;}
        MotorCore_ResetPID(m);m->drive_sign=(int8_t)sign;
    }
    m->stationary_ticks=0;
    /* Opposite-direction samples remain signed PI feedback, not a latched
     * encoder fault. Invalid/stale samples and count bounds still stop. */
    /* No session deadline. Loss of feedback remains a stop condition. */
    if(m->delta!=0 || abs(m->duty)<MOTOR_NO_MOTION_DUTY)m->last_motion=now;
    if((uint32_t)(now-m->last_motion)>=MOTOR_NO_MOTION_MS){MotorCore_Stop(m,MOTOR_STOP_NO_MOTION);return;}
    m->running=1;m->target_mps=speed;
    /* Preserve 8/0.8/0 units: x4 STM32 -> legacy counts/10ms, fractional. */
    float target=fabsf(speed)*10.0f/(ENCODER_EST_MM_PER_COUNT*4.0f);
    float previous_i=m->pid.Iout;
    (void)RT1064_Pid_Calculate(&m->pid,target,(float)(m->delta*sign)/4.0f);
    float upper=(float)(abs(m->duty)+MOTOR_PWM_RISE_STEP)/10.0f;
    if(upper>MOTOR_PWM_LIMIT/10.0f)upper=MOTOR_PWM_LIMIT/10.0f;
    /* PI acts in the requested direction; it cannot apply reverse braking. */
    float candidate=m->pid.Pout+m->pid.Iout+m->pid.Dout;
    float error=m->pid.err[RT1064_Now];
    if(candidate>upper && error>0) {
        float headroom=upper-m->pid.Pout-m->pid.Dout;
        m->pid.Iout=headroom>previous_i?headroom:previous_i;
    } else if(candidate<0 && error<0) {
        float headroom=-m->pid.Pout-m->pid.Dout;
        m->pid.Iout=headroom<previous_i?headroom:previous_i;
    }
    candidate=m->pid.Pout+m->pid.Iout+m->pid.Dout;
    if(!isfinite(candidate)){MotorCore_Stop(m,MOTOR_STOP_FAULT);return;}
    float out=candidate<0?0:(candidate>upper?upper:candidate);
    /* Applied output tracking lets the legacy integral gate recover. */
    m->pid.Out=out;
    m->duty=(int16_t)(sign*out*10.0f); /* original MOTOR_SetPwmValue scale */
}
#endif
