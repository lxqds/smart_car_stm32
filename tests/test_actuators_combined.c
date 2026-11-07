#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "tim.h"
#include "vehicle_app.h"
#include "motor_pwm.h"
static uint32_t now;
uint32_t HAL_GetTick(void){return now;}
uint32_t HAL_RCC_GetHCLKFreq(void){return 240000000;}
uint32_t HAL_RCC_GetPCLK1Freq(void){return 120000000;}
static struct {uint32_t CFGR;} rcc;
#define RCC (&rcc)
#define RCC_CFGR_TIMPRE 1U
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
static TestTim t2,t5;
TIM_HandleTypeDef htim2={&t2,0,0,0},htim5={&t5,0,0,0};
static int failed_timer,encoder_ok=1;
static int32_t encoder_delta=1;
void MX_TIM2_Init(void){}
void MX_TIM5_Init(void){}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *t,int c){
    assert((t==&htim2 && c==2)||(t==&htim5 && c==3));
    return failed_timer==(t==&htim2?2:5);
}
int HAL_TIM_PWM_Stop(TIM_HandleTypeDef *t,int c){(void)t;(void)c;return 0;}
void HAL_GPIO_WritePin(int port,int pin,int value){assert(port==GPIOA && pin==GPIO_PIN_0);(void)value;}
static VehicleRxView rx;
void VehicleApp_GetRxView(VehicleRxView *v){*v=rx;}
void VehicleApp_GetEncoderView(VehicleEncoderView *v){v->delta=encoder_delta;v->valid=encoder_ok;v->sampled_at=now;}
void USB_Log_Event(const char *tag,const char *s){(void)tag;assert(strlen(s)<=48);}
/* Rename internal statics only to include both real drivers in one host TU. */
#define tick servo_tick
#define log_tick servo_log_tick
#define last_rx_at servo_last_rx_at
#include "../Core/Src/servo_control.c"
#undef tick
#undef log_tick
#undef last_rx_at
#include "../Core/Src/vehicle_ports.c"
#include "../Core/Src/motor_pwm.c"
static void stopped(void){assert(!motor.armed && !servo.armed && !htim2.ccr && !htim5.ccr);}
static void reset_at(uint32_t at){
    now=at;encoder_ok=1;rx=(VehicleRxView){0};
    rx.fresh=rx.gear_fresh=1;rx.received_pwm=1500;rx.faults=VEHICLE_FAULT_CONFIG;
    MotorPWM_Init();Servo_Init();stopped();
}
static void advance(float speed,unsigned pwm,int y,int supply){
    now+=10;rx.received_mps=speed;rx.received_pwm=(uint16_t)pwm;rx.display_enabled=y;
    if(supply>=0)MotorPWM_Power(supply,0);
    MotorPWM_Tick();Servo_Tick();
}
static void arm_enable(void){
    MotorPWM_Power(1,1);assert(motor.armed && servo.armed);
    advance(0,1500,1,1);assert(motor.armed && !htim2.ccr);
    advance(0,1500,1,1);assert(servo.active && htim5.ccr==1500);
}
int main(void){
    assert(ACTUATORS_COMBINED && MOTOR_PWM_COMMISSIONING && !VEHICLE_OUTPUTS_ALLOWED);
    reset_at(0);assert(htim2.arr==47999 && htim5.arr==19999 && htim5.psc==239);
    advance(0,1500,1,1);stopped(); /* remote Y cannot arm */
    reset_at(100);rx.received_pwm=1400;MotorPWM_Power(1,1);stopped();
    rx.received_pwm=1500;rx.received_mps=1;MotorPWM_Power(1,1);stopped();
    rx.received_mps=0;encoder_ok=0;MotorPWM_Power(1,1);stopped();
    reset_at(200);arm_enable();
    for(unsigned i=0;i<30;++i)advance(1,1140,1,1);
    assert(motor.running && motor.duty>0 && htim2.ccr>0 && servo.armed && htim5.ccr==1800);
    for(unsigned i=0;i<6;++i)advance(1,1140,1,1);
    assert(htim5.ccr==1860); /* positive throttle does not stop steering */
    encoder_delta=-5;advance(1,1140,1,1);
    assert(motor.armed && servo.armed && servo.active && htim5.ccr==1860);
    encoder_delta=1;
    advance(0,1500,1,1);assert(!htim2.ccr && servo.armed); /* brake/zero keeps steering */
    advance(0,1500,0,1);stopped();
    advance(1,1140,1,1);stopped(); /* no automatic rearm */
    reset_at(1000);arm_enable();encoder_ok=0;advance(1,1140,1,1);stopped();
    reset_at(2000);arm_enable();advance(1,499,1,1);stopped();
    reset_at(3000);arm_enable();advance(NAN,1500,1,1);stopped();
    reset_at(4000);arm_enable();rx.fresh=0;advance(1,1140,1,1);stopped();
    reset_at(5000);arm_enable();rx.gear_fresh=0;advance(1,1140,1,1);stopped();
    reset_at(6000);arm_enable();advance(1,1140,1,0);stopped();
    reset_at(7000);arm_enable();now+=50;MotorPWM_Power(1,0);stopped();
    reset_at(8000);arm_enable();now+=10;advance(1,1140,1,1);stopped(); /* missed control tick */
    reset_at(9000);MotorPWM_Power(1,1);advance(1,1140,1,1);stopped(); /* first Y must center */
    reset_at(9500);arm_enable();
    for(unsigned i=0;i<30;i++)advance(0,1500,1,1);
    for(unsigned i=0;i<20;i++)advance(-0.5f,1140,1,1);
    assert(motor.armed && motor.duty<0 && htim2.ccr>0 && servo.armed && htim5.ccr==1700);
    advance(0.5f,1500,1,1);stopped(); /* reversal guard also removes steering permit */
    reset_at(UINT32_MAX-15U);arm_enable();advance(1,1140,1,1);assert(motor.armed && servo.armed);
    MotorPWM_Abort();stopped();
    failed_timer=2;reset_at(10000);MotorPWM_Power(1,1);stopped();
    failed_timer=5;reset_at(11000);MotorPWM_Power(1,1);stopped();
    failed_timer=0;reset_at(12000);arm_enable();MotorPWM_LogPoll();Servo_LogPoll();
    puts("PASS combined real drivers: simultaneous PI/steering, atomic neutral arm, Y/link/power/encoder/range/cadence stops both, init failure, wrap, motor zero keeps steering");
}
