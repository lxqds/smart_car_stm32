#include "servo_control.h"
#include "motor_pwm.h"
#include "vehicle_app.h"
#include "rt1064_reference.h"
#include "main.h"
#include "tim.h"
#include "usb_log.h"
#include <stdio.h>
#include <math.h>
#if SERVO_OUTPUT_ENABLED && VEHICLE_OUTPUTS_ALLOWED
#error Legacy vehicle output owner must remain disabled
#endif
/* reason: 0 ready, 1 power, 2 link, 3 Y off, 4 range/throttle,
 * 6 cadence, 7 fault, 8 port. No automatic rearm after any stop. */
static ServoView servo;
static uint8_t configured,power_on,enabled_seen;
static uint32_t power_at,tick,last_rx_at,log_tick;
static uint32_t servo_clock(void)
{
    uint32_t h=HAL_RCC_GetHCLKFreq(),p=HAL_RCC_GetPCLK1Freq();
    if(!p || h%p)return 0;
    uint32_t div=h/p;
    if(div!=1 && div!=2 && div!=4 && div!=8 && div!=16)return 0;
    return (RCC->CFGR & RCC_CFGR_TIMPRE)?(div<=4?h:4U*p):(div<=2?h:2U*p);
}
static void stop(unsigned reason)
{
    if(configured){__HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_3,0);htim5.Instance->EGR=TIM_EGR_UG;}
    servo.armed=servo.active=enabled_seen=0;servo.pulse=0;
    servo.target=SERVO_CENTER_US;servo.reason=reason;
}
void Servo_Abort(void)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();
    stop(7);power_on=0;__set_PRIMASK(irq);
}
void Servo_Init(void)
{
    servo.ready=0;Servo_Abort();
    if(!SERVO_BENCH_MODE)return;
    uint32_t clock=servo_clock();
    if(clock<1000000 || clock>480000000 || clock%1000000){servo.reason=8;return;}
    MX_TIM5_Init();
    __HAL_TIM_SET_PRESCALER(&htim5,clock/1000000U-1U);
    __HAL_TIM_SET_AUTORELOAD(&htim5,19999U); /* exactly 50Hz, CCR in us */
    __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_3,0);
    htim5.Instance->EGR=TIM_EGR_UG;configured=1;
    if(HAL_TIM_PWM_Start(&htim5,TIM_CHANNEL_3)!=HAL_OK) {
        stop(8);HAL_TIM_PWM_Stop(&htim5,TIM_CHANNEL_3);return;
    }
    tick=HAL_GetTick();servo.ready=1;servo.reason=0;
    USB_Log_Event("SERVO_MODE","PA2 50Hz MID1500 L1140 R2000 STEP20");
    USB_Log_Event("SERVO_DIR","RT1064 DEFAULT; HOST GAMEPAD INVERTS");
}
static int safe_input(const VehicleRxView *v)
{
    return v->fresh && v->gear_fresh && isfinite(v->received_mps) &&
        (ACTUATORS_COMBINED ? (v->received_mps>=-0.5f && v->received_mps<=5) : v->received_mps==0) &&
        v->received_pwm>=500 && v->received_pwm<=2500 &&
        !(v->faults & (ACTUATORS_COMBINED ? ~VEHICLE_FAULT_CONFIG :
                      (VEHICLE_FAULT_TIMING|VEHICLE_FAULT_NUMERIC)));
}
void Servo_Power(int on,int local_arm)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();uint32_t now=HAL_GetTick();
    /* Check old lease BEFORE a late main-loop update could renew it. */
    if(servo.armed && (uint32_t)(now-power_at)>=50U)stop(1);
    power_on=(uint8_t)(on!=0);power_at=now;
    if(!on)stop(1);
    if(servo.ready && on && local_arm && !servo.armed) {
        VehicleRxView v;VehicleApp_GetRxView(&v);
        if(safe_input(&v) && !v.display_enabled && v.received_pwm==1500 && v.received_mps==0) {
            servo.armed=1;servo.reason=0;enabled_seen=0;
            last_rx_at=now-v.age_ms;
        }
    }
    __set_PRIMASK(irq);
}
void Servo_Tick(void)
{
    if(!servo.ready)return;
    uint32_t now=HAL_GetTick(),elapsed=now-tick;if(elapsed<20U)return;
    uint32_t irq=__get_PRIMASK();__disable_irq();tick=now;
    if(!servo.armed){__set_PRIMASK(irq);return;}
    VehicleRxView v;VehicleApp_GetRxView(&v);
    uint32_t arrived=now-v.age_ms;
    if(!power_on || (uint32_t)(now-power_at)>=50U)stop(1);
    else if(elapsed!=20U)stop(6);
    else if(!v.fresh || !v.gear_fresh || (uint32_t)(arrived-last_rx_at)>=200U)stop(2);
    else if(!safe_input(&v))stop(4);
    else if(!v.display_enabled) {if(enabled_seen)stop(3);}
    else {
        /* First Y enable must still be centered. Do not jump to a held stick. */
        if(!enabled_seen && v.received_pwm!=1500){stop(4);__set_PRIMASK(irq);return;}
        enabled_seen=1;servo.command=v.received_pwm;
        uint16_t target=RT1064_ServoCorrect(v.received_pwm,SERVO_CENTER_US,
                                          SERVO_LEFT_THRESHOLD,SERVO_RIGHT_THRESHOLD);
        servo.target=target;
        if(!servo.active){servo.pulse=SERVO_CENTER_US;servo.active=1;}
        else if(target>servo.pulse) {
            unsigned gap=(unsigned)target-servo.pulse;
            servo.pulse+=(uint16_t)(gap>SERVO_RISE_US?SERVO_RISE_US:gap);
        } else if(target<servo.pulse) {
            unsigned gap=(unsigned)servo.pulse-target;
            servo.pulse-=(uint16_t)(gap>SERVO_RISE_US?SERVO_RISE_US:gap);
        }
        __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_3,servo.pulse);
    }
    last_rx_at=arrived;__set_PRIMASK(irq);
}
void Servo_GetView(ServoView *v)
{
    uint32_t irq=__get_PRIMASK();__disable_irq();*v=servo;__set_PRIMASK(irq);
}
/* Read-only hardware evidence. No timer reset/update or GPIO writes here.
 * Register readback is not a measurement of the waveform at the connector. */
static void servo_hardware_log(uint32_t now)
{
#if defined(STM32H743xx)
    static uint32_t last;
    if(!configured || (uint32_t)(now-last)<1000U)return;
    last=now;
    uint32_t cnt0=TIM5->CNT,cr1=TIM5->CR1,ccer=TIM5->CCER;
    uint32_t ccmr2=TIM5->CCMR2,psc=TIM5->PSC,arr=TIM5->ARR;
    uint32_t ccr=TIM5->CCR3,smcr=TIM5->SMCR,cr2=TIM5->CR2;
    uint32_t moder=GPIOA->MODER,afrl=GPIOA->AFR[0];
    uint32_t apb=RCC->APB1LENR,ahb=RCC->AHB4ENR,cnt1=TIM5->CNT;
    char s[49];
    snprintf(s,sizeof(s),"cr1=%08lX ccer=%08lX",(unsigned long)cr1,(unsigned long)ccer);
    USB_Log_Event("SERVO_HW_CTL",s);
    snprintf(s,sizeof(s),"psc=%lu arr=%lu",(unsigned long)psc,(unsigned long)arr);
    USB_Log_Event("SERVO_HW_RATE",s);
    snprintf(s,sizeof(s),"ccr3=%lu",(unsigned long)ccr);
    USB_Log_Event("SERVO_HW_CCR",s);
    snprintf(s,sizeof(s),"cnt0=%lu cnt1=%lu",(unsigned long)cnt0,(unsigned long)cnt1);
    USB_Log_Event("SERVO_HW_CNT",s);
    snprintf(s,sizeof(s),"ccmr2=%08lX smcr=%08lX",(unsigned long)ccmr2,(unsigned long)smcr);
    USB_Log_Event("SERVO_HW_MODE",s);
    snprintf(s,sizeof(s),"cr2=%08lX",(unsigned long)cr2);
    USB_Log_Event("SERVO_HW_TRIG",s);
    snprintf(s,sizeof(s),"moder=%08lX afrl=%08lX",(unsigned long)moder,(unsigned long)afrl);
    USB_Log_Event("SERVO_HW_GPIO",s);
    snprintf(s,sizeof(s),"apb1l=%08lX ahb4=%08lX",(unsigned long)apb,(unsigned long)ahb);
    USB_Log_Event("SERVO_HW_CLK",s);
#else
    (void)now;
#endif
}
void Servo_LogPoll(void)
{
    uint32_t now=HAL_GetTick();if(!SERVO_BENCH_MODE || (uint32_t)(now-log_tick)<200U)return;log_tick=now;
    ServoView v;Servo_GetView(&v);char s[49];
    snprintf(s,sizeof(s),"ready=%u arm=%u on=%u stop=%lu",v.ready,v.armed,v.active,(unsigned long)v.reason);
    USB_Log_Event("SERVO_STATE",s);
    snprintf(s,sizeof(s),"cmd=%u target=%u out=%u %s",v.command,v.target,v.pulse,
             ACTUATORS_COMBINED ? "COMBINED" : "MOTOR_OFF");
    USB_Log_Event("SERVO_US",s);
    servo_hardware_log(now);
}
