#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "tim.h"
#include "vehicle_app.h"
#include "motor_pwm.h"
#include "servo_control.h"
static MotorSpeedView motor_view;
static ServoView servo_view;
void MotorPWM_GetView(MotorSpeedView *v){*v=motor_view;}
void Servo_GetView(ServoView *v){*v=servo_view;}
static uint32_t now,started_at[32];
static unsigned starts,toggle_count;
static int running,pin_level,fail;
static TestTim timer;
TIM_HandleTypeDef htim13={&timer,0,0,0};
static VehicleRxView rx;
uint32_t HAL_GetTick(void){return now;}
void VehicleApp_GetRxView(VehicleRxView *v){*v=rx;}
void HAL_GPIO_WritePin(int p,unsigned b,int v){assert(p==BEEP_GPIO_Port && b==BEEP_Pin);pin_level=v;}
void HAL_GPIO_TogglePin(int p,unsigned b){assert(p==BEEP_GPIO_Port && b==BEEP_Pin);pin_level=!pin_level;toggle_count++;}
int HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *t){assert(t==&htim13);running=0;return 0;}
int HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *t){assert(t==&htim13);if(fail)return 1;assert(starts<32);started_at[starts++]=now;running=1;return 0;}
#include "../hardware/BEEP/tone.c"
static void poll(uint32_t t,int y){now=t;rx.display_enabled=y;Tone_ControlPoll();}
static void finish_sound(void){while(running)Tone_TimerHandler();assert(pin_level==0);}
int main(void){
    Tone_ControlPoll();assert(starts==0); /* before GPIO/timer init */
    Tone_ControlInit();rx.fresh=rx.gear_fresh=1;
    poll(10,0);poll(20,0);assert(starts==0 && !running);
    poll(30,1);assert(starts==1 && started_at[0]==30 && htim13.arr==554 && toggles==108);
    tone(700,20);assert(starts==1); /* UI click cannot truncate three beeps */
    finish_sound();assert(toggle_count==108);
    for(uint32_t t=40;t<=400;t+=10){poll(t,1);if(running)finish_sound();}
    assert(starts==3 && started_at[1]==150 && started_at[2]==270 && !remaining);
    poll(410,1);assert(starts==3); /* repeated fresh Y packets */
    poll(420,0);finish_sound();
    for(uint32_t t=430;t<=700;t+=10){poll(t,0);if(running)finish_sound();}
    assert(starts==5 && started_at[3]==420 && started_at[4]==560 && !remaining);
    tone(700,20);assert(starts==6 && toggles==28);finish_sound();
    poll(710,1);assert(remaining==3);
    poll(720,0);assert(remaining==2 && interval_ms==70); /* off preempts startup */
    noTone();assert(!running && !remaining && !pin_level);
    poll(730,1);rx.fresh=0;poll(740,1);assert(alarm_active && interval_ms==500 && !control_enabled);
    noTone();rx.fresh=1;rx.gear_fresh=0;poll(750,1);assert(!remaining);
    now=UINT32_MAX-20U;Tone_ControlInit();rx.fresh=rx.gear_fresh=1;
    poll(UINT32_MAX-10U,1);unsigned before=starts;
    poll(49,1);assert(!sounding);poll(119,1);assert(starts==before+1); /* wrap */
    noTone();fail=1;poll(129,0);assert(!running && !pin_level);
    fail=0;noTone();tone(0,20);tone(700,0);tone(500001,20);assert(!running);
    tone(900,TONE_DURATION_INFINITE);assert(running);Tone_TimerHandler();noTone();assert(!running && !pin_level);
    now=1000;rx=(VehicleRxView){0};rx.fresh=rx.gear_fresh=1;
    Tone_ControlInit();unsigned count=starts;
    rx.gamepad_connected=1;poll(1010,0);assert(starts==count+1 && toggles==126);
    finish_sound();for(unsigned t=1020;t<=1200;t+=10)poll(t,0);
    assert(starts==count+1); /* repeated health packets are silent */
    rx.gamepad_connected=0;poll(1210,0);rx.gamepad_connected=1;poll(1220,0);
    assert(starts==count+2);noTone();
    motor_view.reason=9;poll(1230,0);assert(alarm_active && toggles==900 && interval_ms==500);
    count=starts;poll(1240,1);tone(700,20);assert(starts==count && alarm_active);
    rx.gamepad_connected=0;poll(1250,1);rx.gamepad_connected=1;poll(1260,1);
    assert(starts==count && pending_connection); /* connection waits behind alarm */
    finish_sound();poll(1720,1);assert(alarm_active);poll(1730,1);
    assert(!alarm_active && !remaining && !running && !pin_level);
    poll(1740,1);assert(starts==count+1 && interval_ms==70 && !alarm_active);
    noTone(); /* persistent stop did not restart alarm; only queued connect beep */
    motor_view.reason=0;servo_view.reason=0;poll(1750,1);
    motor_view.reason=3;servo_view.reason=7;poll(1760,0);
    assert(!alarm_active && remaining==2); /* normal Y peer shutdown is not a fault */
    noTone();motor_view.reason=0;servo_view.reason=0;poll(1770,1);
    motor_view.reason=3;servo_view.reason=7;rx.gamepad_connected=0;poll(1780,0);
    assert(alarm_active && interval_ms==500); /* wireless loss can arrive as disabled Y */
    noTone();
    puts("PASS real PD11/TIM13 tone: boot quiet, ON 3x60ms/OFF 2x70ms, duplicate suppression, preemption, stale link, UI priority, wrap, finite waveform and HAL failure");
}
