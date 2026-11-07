#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "tim.h"
#include "vehicle_app.h"
#include "motor_pwm_core.h"
static uint32_t now;
uint32_t HAL_GetTick(void){return now;}
static uint32_t hclk=240000000,pclk=120000000;
uint32_t HAL_RCC_GetHCLKFreq(void){return hclk;}
uint32_t HAL_RCC_GetPCLK1Freq(void){return pclk;}
static struct {uint32_t CFGR;} rcc;
#define RCC (&rcc)
#define RCC_CFGR_TIMPRE 1U
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
static TestTim t2,t5;
TIM_HandleTypeDef htim2={&t2,0,0,0},htim5={&t5,0,0,0};
static int fail,start_count,dir;
void MX_TIM2_Init(void){}
void MX_TIM5_Init(void){assert(0);}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *t,int c){assert(t==&htim2 && c==2);start_count++;return fail;}
int HAL_TIM_PWM_Stop(TIM_HandleTypeDef *t,int c){assert(t==&htim2 && c==2);return 0;}
void HAL_GPIO_WritePin(int port,int pin,int v){assert(port==GPIOA && pin==GPIO_PIN_0);dir=v;}
static VehicleRxView view;
static int32_t encoder_delta;
static int encoder_ok=1;
static uint32_t encoder_age;
void VehicleApp_GetEncoderView(VehicleEncoderView *e){e->delta=encoder_delta;e->valid=encoder_ok;e->sampled_at=now-encoder_age;}
void VehicleApp_GetRxView(VehicleRxView *v){*v=view;}
static unsigned log_count;
static char speed_line[49],terms_line[49];
void USB_Log_Event(const char *tag,const char *s){assert(strlen(s)<=48);log_count++;if(!strcmp(tag,"SPEED_PI"))strcpy(speed_line,s);if(!strcmp(tag,"PI_TERMS20K"))strcpy(terms_line,s);}
#include "../Core/Src/vehicle_ports.c"
#include "../Core/Src/motor_pwm.c"
static void step(uint32_t t,float speed,int y,int power,int fresh)
{
    now=t;view.received_mps=speed;view.display_enabled=y;
    view.fresh=view.gear_fresh=fresh;view.age_ms=0;
    if(power>=0)MotorPWM_Power(power,0);
    MotorPWM_Tick();
}
static void arm(uint32_t t)
{
    now=t;tick=t;view.received_mps=0;view.display_enabled=0;view.age_ms=0;
    view.fresh=view.gear_fresh=1;view.faults=VEHICLE_FAULT_CONFIG;
    MotorPWM_Power(1,1);assert(motor.armed);
}
int main(void)
{
    MotorPWM_Init();assert(ready && start_count==1 && htim2.arr==47999 && htim2.psc==0);
    assert(!motor.armed && !htim2.ccr && !htim5.ccr);
    step(10,5,1,1,1);assert(!htim2.ccr); /* remote Y cannot arm */
    now=20;MotorPWM_Power(1,1);assert(!motor.armed); /* nonneutral */
    arm(20);
    step(30,5,1,1,1);assert(motor.duty==10 && htim2.ccr==0);
    for(unsigned t=40;t<=130;t+=10)step(t,5,1,1,1);
    assert(motor.duty==110 && htim2.ccr==264 && !htim5.ccr && dir==1);
    step(140,0,1,1,1);assert(!htim2.ccr); /* zero immediate */
    step(150,1,0,1,1);assert(!motor.armed && !htim2.ccr);
    step(160,5,1,1,1);assert(!htim2.ccr); /* Y cannot rearm */
    arm(170);step(180,5,1,1,1);step(190,5,1,0,1);
    assert(!motor.armed && !htim2.ccr);
    step(200,5,1,1,1);assert(!htim2.ccr);
    arm(210);step(220,5,1,1,1);step(270,5,1,-1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_POWER && !htim2.ccr);
    arm(280);step(290,5,1,1,1);step(300,5,1,1,0);
    assert(!motor.armed && motor.reason==MOTOR_STOP_LINK);
    arm(310);step(320,-1,1,1,1);assert(!motor.armed && motor.reason==MOTOR_STOP_RANGE);
    arm(330);step(340,NAN,1,1,1);assert(!motor.armed);
    arm(350);step(370,1,1,1,1);assert(!motor.armed && motor.reason==MOTOR_STOP_TIMING);
    arm(380);view.faults=VEHICLE_FAULT_TIMING;step(390,1,1,1,1);assert(!motor.armed);
    arm(400);encoder_delta=1;
    for(unsigned t=410;t<=20410;t+=10)step(t,5,1,1,1);
    assert(motor.armed && motor.running && motor.duty==19500); /* no 15s deadline */
    encoder_delta=0;MotorPWM_Abort();
    arm(2430);MotorPWM_Abort();assert(!motor.armed && !motor.power && !htim2.ccr);
    /* Arrival gap cannot renew old permission between periodic checks. */
    arm(2440);last_rx_at=2240;step(2450,1,1,1,1);assert(!motor.armed);
    arm(2460);step(2470,0,1,1,1);assert(motor.armed && !motor.running && !htim2.ccr);
    step(2480,0,0,1,1);assert(!motor.armed); /* Y off disarms even without throttle */
    /* Restore the original 1950*10 rail, including the hardware-facing clamp. */
    MotorPort_Apply(32000);MotorPort_Apply(32000);assert(htim2.ccr==46800);
    MotorPort_Apply(15000);assert(htim2.ccr==36000); /* no hidden 50% clamp */
    MotorPort_Apply(-1000);assert(!htim2.ccr);
    MotorPort_Apply(-1000);assert(!htim2.ccr);
    MotorPort_Apply(-1000);assert(htim2.ccr==2400 && dir==0);
    MotorPWM_Abort();
    /* Percentages and low increments survive the 20000 normalization. */
    MotorPort_Apply(100);MotorPort_Apply(100);assert(htim2.ccr==240); /* 0.5% */
    MotorPort_Apply(1);assert(htim2.ccr==2); /* floor(48000/20000), nonzero */
    MotorPort_Apply(0);assert(!htim2.ccr);
    assert(!MotorPort_Configure(240000001,5000)); /* reject frequency rounding */
    assert(MotorPort_Configure(480000000,1000));
    /* At this period, multiplication by full scale exceeds uint32. */
    apply_motor(20000);apply_motor(20000);assert(htim2.ccr==480000);
    apply_motor(10000);assert(htim2.ccr==240000);
    assert(MotorPort_Configure(240000000,5000));
    assert(htim2.psc==0 && htim2.arr==47999 && !htim2.ccr);
    MotorPWM_Abort();
    /* Original position-PID arithmetic in legacy counts, unsaturated. */
    MotorPWMCore m={0};MotorCore_Feedback(&m,100,1);
    uint32_t base=UINT32_MAX-50U;
    MotorCore_Power(&m,1,base);assert(MotorCore_Arm(&m,base,1,0,0));
    m.duty=10000;
    RT1064_PID ref=m.pid;
    float out=RT1064_Pid_Calculate(&ref,0.4f*10/(ENCODER_EST_MM_PER_COUNT*4),25.0f);
    MotorCore_Step(&m,base,10,1,1,0.4f,0);
    assert(m.duty==(int16_t)(out*10) && m.pid.Iout==ref.Iout);
    /* Fractional x4 conversion: one count must not become zero. */
    MotorCore_Feedback(&m,1,1);m.duty=10000;
    ref=m.pid;out=RT1064_Pid_Calculate(&ref,0.1f*10/(ENCODER_EST_MM_PER_COUNT*4),0.25f);
    MotorCore_Power(&m,1,base+10);MotorCore_Step(&m,base+10,10,1,1,0.1f,0);
    assert(m.duty==(int16_t)(out*10));
    /* Feedback, not a speed-to-PWM table: overspeed cannot reverse the motor. */
    MotorCore_Feedback(&m,500,1);MotorCore_Power(&m,1,base+20);
    MotorCore_Step(&m,base+20,10,1,1,0.1f,0);assert(m.duty==0);
    /* Core wrap, persistent operation, hardware cap and ramp-aware anti-windup. */
    MotorPWMCore ramp={0};MotorCore_Feedback(&ramp,1,1);MotorCore_Power(&ramp,1,base);
    assert(MotorCore_Arm(&ramp,base,1,0,0));
    for(unsigned dt=10;dt<=20000;dt+=10) {
        int before=ramp.duty;
        MotorCore_Power(&ramp,1,base+dt);MotorCore_Step(&ramp,base+dt,10,1,1,5,0);
        assert(ramp.duty-before<=10 && ramp.duty<=19500 && ramp.armed);
        assert(ramp.pid.Iout==0); /* no wind-up against rise limit or output cap */
    }
    assert(ramp.duty==19500);
    MotorCore_Feedback(&ramp,500,1);MotorCore_Power(&ramp,1,base+20010);
    MotorCore_Step(&ramp,base+20010,10,1,1,0.5f,0);assert(ramp.duty==0);
    MotorCore_Feedback(&ramp,1,1);MotorCore_Power(&ramp,1,base+20020);
    MotorCore_Step(&ramp,base+20020,10,1,1,1,0);assert(ramp.duty>0 && ramp.duty<=10);
    MotorCore_Power(&ramp,1,base+20030);MotorCore_Step(&ramp,base+20030,10,1,0,1,0);
    assert(!ramp.armed && ramp.duty==0 && ramp.pid.Iout==0 && ramp.pid.err[RT1064_Last]==0);
    /* Reverse PI uses signed feedback, requires a stopped neutral interval. */
    MotorPWMCore rev={0};MotorCore_Feedback(&rev,0,1);MotorCore_Power(&rev,1,0);
    assert(MotorCore_Arm(&rev,0,1,0,0));
    MotorCore_Step(&rev,10,10,1,1,-0.5f,0);
    assert(!rev.armed && rev.reason==MOTOR_STOP_DIRECTION);
    MotorCore_Power(&rev,1,20);assert(MotorCore_Arm(&rev,20,1,0,0));
    for(unsigned t=30;t<=320;t+=10){MotorCore_Power(&rev,1,t);MotorCore_Step(&rev,t,10,1,1,0,0);}
    MotorCore_Power(&rev,1,330);MotorCore_Step(&rev,330,10,1,1,-0.5f,0);
    assert(rev.armed && rev.duty==-10 && rev.target_mps==-0.5f);
    MotorCore_Feedback(&rev,-1,1);
    MotorCore_Power(&rev,1,340);MotorCore_Step(&rev,340,10,1,1,-0.5f,0);
    assert(rev.armed && rev.duty<0);
    /* A forward sample during reverse control must not revoke permission. */
    MotorCore_Feedback(&rev,5,1);
    int reverse_before=rev.duty;
    MotorCore_Power(&rev,1,345);MotorCore_Step(&rev,345,10,1,1,-0.5f,0);
    assert(rev.armed && rev.reason==0 && rev.measured_mps>0);
    assert(rev.duty<=0 && abs(rev.duty)<=abs(reverse_before)+MOTOR_PWM_RISE_STEP);
    MotorCore_Power(&rev,1,350);MotorCore_Step(&rev,350,10,1,1,0.5f,0);
    assert(!rev.armed && !rev.duty && rev.reason==MOTOR_STOP_DIRECTION);
    MotorCore_Feedback(&rev,5,1);MotorCore_Power(&rev,1,400);
    assert(MotorCore_Arm(&rev,400,1,0,0));
    for(unsigned t=410;t<=810;t+=10){MotorCore_Power(&rev,1,t);MotorCore_Step(&rev,t,10,1,1,0,0);}
    assert(rev.stationary_ticks==0); /* zero command alone is not stationary */
    MotorCore_Power(&rev,1,820);MotorCore_Step(&rev,820,10,1,1,-0.5f,0);
    assert(!rev.armed && rev.reason==MOTOR_STOP_DIRECTION);
    /* Invalid/stale feedback still revokes local permission. */
    arm(30000);encoder_ok=0;step(30010,1,1,1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_ENCODER);encoder_ok=1;
    arm(30020);encoder_age=20;step(30030,1,1,1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_ENCODER);encoder_age=0;
    arm(30040);encoder_delta=-5;step(30050,1,1,1,1);
    assert(motor.armed && motor.running && motor.reason==0 && motor.measured_mps<0);
    assert(motor.duty>=0 && motor.duty<=MOTOR_PWM_RISE_STEP);
    encoder_delta=5;step(30060,1,1,1,1);
    assert(motor.armed && motor.duty>=0 && motor.duty<=2*MOTOR_PWM_RISE_STEP);
    encoder_delta=-5;step(30070,0,1,1,1);
    assert(motor.armed && !motor.running && !motor.duty && motor.pid.Iout==0);
    encoder_delta=0;MotorPWM_Abort();
    arm(30060);encoder_delta=4096;step(30070,1,1,1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_ENCODER);encoder_delta=0;
    arm(30080);encoder_delta=-4096;step(30090,1,1,1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_ENCODER);encoder_delta=0;
    /* Persistent nonzero PWM but no encoder edges stops and cannot auto-rearm. */
    arm(31000);
    for(unsigned t=31010;t<=34000;t+=10)step(t,5,1,1,1);
    assert(!motor.armed && motor.reason==MOTOR_STOP_NO_MOTION && !htim2.ccr);
    encoder_delta=20;step(34010,1,1,1,1);assert(!motor.armed && !htim2.ccr);
    arm(34020);step(34030,1,1,1,1);step(34040,0,1,1,1);
    assert(motor.armed && !motor.running && !motor.duty && motor.pid.Iout==0);
    puts("PASS RT1064 speed PI: units/gains, signed transient feedback without latch, ramp/cap antiwindup, recovery, no deadline, wrap, encoder loss/jump, zero/Y reset");
    /* Simple deterministic plant: verifies feedback response, not physical gains. */
    MotorPWMCore plant={0};MotorCore_Feedback(&plant,0,1);MotorCore_Power(&plant,1,0);
    assert(MotorCore_Arm(&plant,0,1,0,0));float velocity=0;int light_duty=0;
    for(unsigned t=10;t<=40000;t+=10) {
        MotorCore_Feedback(&plant,(int32_t)roundf(velocity/Encoder_EstSpeedMps(1,10)),1);
        MotorCore_Power(&plant,1,t);MotorCore_Step(&plant,t,10,1,1,0.5f,0);
        assert(plant.armed && plant.duty>=0 && plant.duty<=19500);
        velocity+=0.02f*(plant.duty*(t<=20000?0.0003f:0.00015f)-velocity);
        if(t==20000){assert(fabsf(velocity-0.5f)<0.05f);light_duty=plant.duty;}
    }
    assert(fabsf(velocity-0.5f)<0.05f && plant.duty>light_duty);
    puts("PASS simulated speed tracking and load increase with variable PWM (not hardware validation)");
    /* Actual runtime formatter, full uint32 sequence and longest signed fields. */
    now=50000;sample_sequence=UINT32_MAX;motor.target_mps=5;motor.measured_mps=-14.894f;
    motor.duty=10000;motor.pid.Pout=-4000;motor.pid.Iout=4000;motor.pid.Dout=0;
    MotorPWM_LogPoll();
    assert(!strcmp(speed_line,"n=4294967295 t=5000 v=-14894 u=10000"));
    assert(!strcmp(terms_line,"p=-40000 i=40000 d=0"));
    puts("PASS real speed log formatting: full sequence, negative feedback and PI terms");
    for(unsigned div=1;div<=16;div*=2){pclk=hclk/div;rcc.CFGR=0;assert(motor_clock()==(div<=2?hclk:pclk*2));rcc.CFGR=1;assert(motor_clock()==(div<=4?hclk:pclk*4));}
    pclk=hclk/2;fail=1;MotorPWM_Init();assert(!ready && !htim2.ccr && !htim5.ccr);
    now=4000;MotorPWM_LogPoll();assert(log_count>=4);
    puts("PASS motor main integration: motor-only timer, neutral local arm, Y/ramp/cap/zero, power lease, lost link, faults, wrap, RCC, init failure");
}

