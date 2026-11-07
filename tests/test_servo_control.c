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
static int fail,starts;
void MX_TIM2_Init(void){assert(0);}
void MX_TIM5_Init(void){}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *t,int c){assert(t==&htim5 && c==3);starts++;return fail;}
int HAL_TIM_PWM_Stop(TIM_HandleTypeDef *t,int c){assert(t==&htim5 && c==3);return 0;}
static VehicleRxView rx;
void VehicleApp_GetRxView(VehicleRxView *v){*v=rx;}
static char state_log[49],pulse_log[49];
void USB_Log_Event(const char *tag,const char *s){assert(strlen(s)<=48);if(!strcmp(tag,"SERVO_US"))strcpy(pulse_log,s);if(!strcmp(tag,"SERVO_STATE"))strcpy(state_log,s);}
#include "../Core/Src/servo_control.c"
static void arm_at(uint32_t t)
{
    now=t;tick=t;rx=(VehicleRxView){0};rx.fresh=rx.gear_fresh=1;
    rx.received_pwm=1500;rx.faults=VEHICLE_FAULT_CONFIG;
    Servo_Power(1,1);assert(servo.armed && !servo.active && !htim5.ccr);
}
static void step(uint32_t t,int y,unsigned command,int supply)
{
    now=t;rx.display_enabled=y;rx.received_pwm=(uint16_t)command;
    if(supply>=0)Servo_Power(supply,0);
    Servo_Tick();assert(!htim2.ccr);
}
int main(void)
{
    assert(MOTOR_PWM_COMMISSIONING==0 && VEHICLE_OUTPUTS_ALLOWED==0);
    assert(RT1064_ServoCorrect(1140,SERVO_CENTER_US,SERVO_LEFT_THRESHOLD,SERVO_RIGHT_THRESHOLD)==1860);
    assert(RT1064_ServoCorrect(2000,SERVO_CENTER_US,SERVO_LEFT_THRESHOLD,SERVO_RIGHT_THRESHOLD)==1000);
    Servo_Init();assert(servo.ready && starts==1 && htim5.psc==239 && htim5.arr==19999 && !htim5.ccr);
    /* Reconstruct the documented D882 mapping, not an archived old binary.
     * Moving the mirror to the host must preserve every whole-us command
     * in the previously exercised gamepad range. Slew rate is tested below. */
    for(unsigned old_command=1455;old_command<=1545;++old_command) {
        unsigned old_target=3000U-RT1064_ServoCorrect(old_command,1500,1140,2000);
        if(old_target<1450)old_target=1450;
        if(old_target>1550)old_target=1550;
        Servo_Abort();arm_at(1000);step(1020,1,1500,1);
        for(unsigned t=1040;t<=2040;t+=20) {
            step(t,1,3000U-old_command,1);
            assert(servo.target==old_target);
        }
        assert(htim5.ccr==old_target);
    }
    /* Exact step response, both directions and reversal, with no overshoot. */
    Servo_Abort();arm_at(1000);step(1020,1,1500,1);
    for(unsigned i=1;i<=25;++i) {
        step(1020+i*20,1,2000,1);
        assert(htim5.ccr==1500-20*i); /* midpoint -> 1000 in 500ms */
    }
    for(unsigned i=1;i<=43;++i) {
        step(1520+i*20,1,1140,1);
        assert(htim5.ccr==1000+20*i); /* full reversal in 860ms */
    }
    for(unsigned i=1;i<=18;++i) {
        step(2380+i*20,1,1500,1);
        assert(htim5.ccr==1860-20*i); /* return from 1860 in 360ms */
    }
    step(2760,1,1493,1);assert(htim5.ccr==1507); /* short move never overshoots */
    step(2780,1,2000,1);assert(htim5.ccr==1487);
    step(2800,0,1500,1);assert(!htim5.ccr && !servo.armed); /* stop bypasses slew */
    Servo_Abort();tick=0;
    step(20,1,1500,1);assert(!servo.armed && !htim5.ccr); /* remote Y cannot arm */
    Servo_Power(1,1);assert(!servo.armed); /* Y held */
    rx.display_enabled=0;rx.received_pwm=1470;Servo_Power(1,1);assert(!servo.armed);
    arm_at(40);step(60,1,1500,1);assert(htim5.ccr==1500 && servo.active);
    step(80,1,1470,1);assert(servo.target==1530 && htim5.ccr==1520);
    for(unsigned t=100;t<=700;t+=20)step(t,1,1470,1);
    assert(htim5.ccr==1530); /* unchanged RT1064 protocol direction */
    for(unsigned t=720;t<=10000;t+=20)step(t,1,500,1);
    assert(htim5.ccr==1860);
    for(unsigned t=10020;t<=30000;t+=20)step(t,1,2500,1);
    assert(htim5.ccr==1000);
    step(30020,0,1500,1);assert(!servo.armed && !htim5.ccr && servo.reason==3);
    step(30040,1,1500,1);assert(!htim5.ccr);
    arm_at(3500);step(3520,1,1470,1);assert(!servo.armed && servo.reason==4); /* first Y must center */
    arm_at(3540);rx.received_mps=0.1f;step(3560,1,1500,1);assert(!servo.armed && !htim5.ccr);
    arm_at(3580);step(3600,1,1500,1);step(3620,1,1500,0);assert(!servo.armed && !htim5.ccr);
    arm_at(3640);step(3660,1,1500,1);rx.fresh=0;step(3680,1,1500,1);assert(servo.reason==2 && !htim5.ccr);
    arm_at(3700);rx.gear_fresh=0;step(3720,1,1500,1);assert(servo.reason==2);
    arm_at(3740);last_rx_at=3540;step(3760,1,1500,1);assert(servo.reason==2 && !servo.armed);
    arm_at(3780);step(3800,1,499,1);assert(servo.reason==4 && !htim5.ccr);
    arm_at(3820);rx.received_mps=NAN;step(3840,1,1500,1);assert(servo.reason==4);
    arm_at(3860);step(3890,1,1500,1);assert(servo.reason==6 && !servo.armed);
    arm_at(3900);step(3920,1,1500,1);now=3970;Servo_Power(1,0);
    assert(!servo.armed && !htim5.ccr); /* late power renewal cannot hide expired lease */
    arm_at(4000);step(4020,1,1500,1);step(4080,1,1500,-1);assert(servo.reason==1);
    arm_at(UINT32_MAX-10U);step(9,1,1500,1);assert(servo.active && htim5.ccr==1500);
    Servo_Abort();assert(!servo.armed && !htim5.ccr);
    now=5000;servo.command=65535;servo.target=1550;servo.pulse=1450;
    servo.reason=UINT32_MAX;Servo_LogPoll();
    assert(!strcmp(pulse_log,"cmd=65535 target=1550 out=1450 MOTOR_OFF"));
    assert(strstr(state_log,"stop=4294967295"));
    Servo_Abort();fail=1;Servo_Init();assert(!servo.ready && !htim5.ccr && !htim2.ccr);
    puts("PASS servo real integration: PA2 50Hz, boot zero, local neutral/Y gates, RT correction, legacy 1140/2000 thresholds (1000..1860 output), slew, link/power/throttle/fault/rollover, logs, motor untouched");
}

