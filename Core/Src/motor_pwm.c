#include "motor_pwm.h"
#include "motor_pwm_core.h"
#include "vehicle_ports.h"
#include "vehicle_app.h"
#include "main.h"
#include "usb_log.h"
#include "servo_control.h"
#include <stdio.h>
#if MOTOR_PWM_COMMISSIONING && VEHICLE_OUTPUTS_ALLOWED
#error Motor commissioning and vehicle PID must not own PWM concurrently
#endif
static MotorPWMCore motor;
static volatile uint8_t ready;
static uint32_t tick,log_tick,speed_log_tick,timer_hz,last_rx_at,sample_sequence;
static void update_feedback(uint32_t now)
{
    VehicleEncoderView e;VehicleApp_GetEncoderView(&e);
    MotorCore_Feedback(&motor,e.delta,e.valid && (uint32_t)(now-e.sampled_at)<20U);
}
/* HAL documents TIMPRE: HCLK for APB divisors <=2 (or <=4 with TIMPRE),
 * otherwise 2*PCLK (or 4*PCLK). Derive from actual clocks, not a fixed 240MHz. */
static uint32_t motor_clock(void)
{
    uint32_t h=HAL_RCC_GetHCLKFreq(),p=HAL_RCC_GetPCLK1Freq();
    if(!p || h%p)return 0;
    uint32_t div=h/p;
    if(div!=1 && div!=2 && div!=4 && div!=8 && div!=16)return 0;
    return (RCC->CFGR & RCC_CFGR_TIMPRE) ? (div<=4?h:4U*p) : (div<=2?h:2U*p);
}
void MotorPWM_Init(void)
{
    ready=0;MotorPWM_Abort();timer_hz=motor_clock();
    if(!MOTOR_PWM_COMMISSIONING || !MotorPort_Configure(timer_hz,MOTOR_PWM_HZ)) {
        motor.reason=MOTOR_STOP_PORT;return;
    }
    tick=HAL_GetTick();motor.reason=MOTOR_STOP_BOOT;ready=1;
    USB_Log_Event("MOTOR_MODE",ACTUATORS_COMBINED ? "RT1064_SPEED_PI MAX97.5PCT SERVO_ON" : "RT1064_SPEED_PI MAX97.5PCT SERVO_OFF");
    USB_Log_Event("MOTOR_GAINS","KP=8 KI=0.8 KD=0 COUNTS10MS x1");
}
void MotorPWM_Abort(void)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();
    MotorCore_Stop(&motor,MOTOR_STOP_FAULT);motor.power=0;
    VehiclePorts_Disable();
#if SERVO_BENCH_MODE
    Servo_Abort();
#endif
    __set_PRIMASK(irq);
}
void MotorPWM_Power(int on,int local_arm)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();
    uint32_t now=HAL_GetTick();
    /* An overdue main-loop lease must not be renewed before inspection. */
    if(motor.armed && (uint32_t)(now-motor.power_at)>=50U) {
        MotorCore_Stop(&motor,MOTOR_STOP_POWER);VehiclePorts_Disable();
    }
#if SERVO_OUTPUT_ENABLED
    Servo_Power(on,local_arm);
#endif
    update_feedback(now);
    MotorCore_Power(&motor,on,now);
    if(!on)VehiclePorts_Disable();
    if(ready && local_arm) {
        VehicleRxView v;VehicleApp_GetRxView(&v);
        int steering_ok=1;
#if ACTUATORS_COMBINED
        ServoView s;Servo_GetView(&s);
        steering_ok=s.ready && s.armed && v.received_pwm==1500;
#endif
        if(steering_ok && !(v.faults & ~VEHICLE_FAULT_CONFIG))
            if(MotorCore_Arm(&motor,now,v.fresh && v.gear_fresh,v.display_enabled,v.received_mps))
                last_rx_at=now-v.age_ms;
    }
#if ACTUATORS_COMBINED
    /* Arming is all-or-nothing; failed encoder/port/neutral checks revoke both. */
    ServoView s;Servo_GetView(&s);
    if(motor.armed != s.armed) {
        MotorCore_Stop(&motor,MOTOR_STOP_FAULT);VehiclePorts_Disable();Servo_Abort();
    }
#endif
    __set_PRIMASK(irq);
}
void MotorPWM_Tick(void)
{
    if(!ready)return;
    uint32_t now=HAL_GetTick(),elapsed=now-tick;
    if(elapsed<10U)return;
    uint32_t irq=__get_PRIMASK();__disable_irq();
    tick=now;
#if ACTUATORS_COMBINED
    Servo_Tick(); /* inspect steering before applying any motor output */
#endif
    update_feedback(now);sample_sequence++;
    VehicleRxView v;VehicleApp_GetRxView(&v);
    uint32_t arrived=now-v.age_ms;
    if(motor.armed && (uint32_t)(arrived-last_rx_at)>=200U)
        MotorCore_Stop(&motor,MOTOR_STOP_LINK);
    last_rx_at=arrived;
#if ACTUATORS_COMBINED
    ServoView s;Servo_GetView(&s);
    if(motor.armed && (!s.ready || !s.armed ||
       (v.display_enabled && !s.active && (v.received_mps!=0 || v.received_pwm!=1500)) ||
       v.received_pwm<500 || v.received_pwm>2500))
        MotorCore_Stop(&motor,MOTOR_STOP_FAULT);
#endif
    MotorCore_Step(&motor,now,elapsed,v.fresh && v.gear_fresh,
                   v.display_enabled,v.received_mps,(v.faults & ~VEHICLE_FAULT_CONFIG)!=0);
    if(!motor.armed || !motor.duty)VehiclePorts_Disable();
    else MotorPort_Apply(motor.duty);
#if ACTUATORS_COMBINED
    if(!motor.armed && s.armed)Servo_Abort();
#endif
    __set_PRIMASK(irq);
}
void MotorPWM_GetView(MotorSpeedView *v)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();
    v->target_mps=motor.target_mps;v->measured_mps=motor.measured_mps;
    v->duty=motor.duty;v->armed=motor.armed;v->running=motor.running;v->reason=motor.reason;
    __set_PRIMASK(irq);
}
void MotorPWM_LogPoll(void)
{
    if(!MOTOR_PWM_COMMISSIONING)return;
    uint32_t now=HAL_GetTick();if((uint32_t)(now-speed_log_tick)<50U)return;speed_log_tick=now;
    uint32_t irq=__get_PRIMASK();__disable_irq();
    MotorPWMCore m=motor;uint32_t seq=sample_sequence;
    __set_PRIMASK(irq);
    char s[49];
    /* Bounded mm/s and normalized duty, no printf in the control ISR.
     * Sequence counts 10ms updates; missing samples/logs remain observable. */
    snprintf(s,sizeof(s),"n=%lu t=%ld v=%ld u=%d",(unsigned long)seq,
             (long)(m.target_mps*1000),(long)(m.measured_mps*1000),m.duty);
    USB_Log_Event("SPEED_PI",s);
    if((uint32_t)(now-log_tick)<500U)return;log_tick=now;
    snprintf(s,sizeof(s),"ready=%u arm=%u run=%u req20k=%d",ready,m.armed,m.running,m.duty);
    USB_Log_Event("MOTOR_PWM",s);
    snprintf(s,sizeof(s),"reason=%lu pwr=%u hz=%lu",(unsigned long)m.reason,m.power,(unsigned long)MOTOR_PWM_HZ);
    USB_Log_Event("MOTOR_GUARD",s);
    snprintf(s,sizeof(s),"timer_hz=%lu",(unsigned long)timer_hz);
    USB_Log_Event("MOTOR_CLOCK",s);
    snprintf(s,sizeof(s),"ccr=%lu arr=%lu",(unsigned long)MotorPort_Compare(),(unsigned long)MotorPort_Period());
    USB_Log_Event("MOTOR_REG",s);
    snprintf(s,sizeof(s),"p=%ld i=%ld d=%ld",(long)(m.pid.Pout*10),
             (long)(m.pid.Iout*10),(long)(m.pid.Dout*10));
    USB_Log_Event("PI_TERMS20K",s);
}
