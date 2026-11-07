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
static unsigned servo_power_calls,servo_abort_calls;
void Servo_Power(int on,int arm){(void)on;(void)arm;servo_power_calls++;}
void Servo_Abort(void){servo_abort_calls++;}
#include "../Core/Src/motor_pwm.c"
int main(void){
 assert(SERVO_BENCH_MODE && !MOTOR_PWM_COMMISSIONING);
 MotorPWM_Init();assert(!ready && !start_count && !htim2.ccr);
 view.fresh=view.gear_fresh=1;view.received_mps=0;
 MotorPWM_Power(1,1);view.display_enabled=1;view.received_mps=5;
 for(now=10;now<1000;now+=10){MotorPWM_Power(1,0);MotorPWM_Tick();MotorPort_Apply(20000);}
 assert(!motor.armed && !start_count && !htim2.ccr && !htim5.ccr);
 assert(!MotorPort_Configure(240000000,5000));
 MotorPWM_Abort();assert(servo_power_calls && servo_abort_calls);
 puts("PASS servo-build motor exclusion: no TIM2 init/start/duty, power/abort forwarded to servo");
}
