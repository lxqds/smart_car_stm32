/* Executes the exact Power_Service/callback block extracted from main.c. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "power_control.h"
#include "power_log.h"
#include "dac_safe.h"
#define POWER_OUTPUT_ENABLED 1
#define HAL_OK 0
#define HAL_ERROR 1
#define DAC_CHANNEL_1 1
#define DAC_CHANNEL_2 2
#define DAC_ALIGN_12B_R 0
#define LM5116_EN_GPIO_Port 1
#define LM5116_EN_Pin 1
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
typedef int HAL_StatusTypeDef;
typedef int ADC_HandleTypeDef;
static int hadc1,hdac1;
static volatile uint8_t key;
static uint8_t LM5116_EN;
static float U_set=12,I_set=1,DAC_set,DAC_set_I;
static volatile uint16_t ADC_value[5];
static uint32_t tick;
static int enabled,writes,fail_dac,adc_error_on_write;
uint32_t HAL_GetTick(void){return tick;}
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
static void HAL_GPIO_WritePin(int port,int pin,int value)
{assert(port==1 && pin==1);if(value && !enabled)assert(writes>=2);enabled=value;}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *h);
static int HAL_DAC_SetValue(int *h,int channel,int align,uint16_t code)
{
    (void)h;(void)align;assert(channel==1 || channel==2);assert(code<=4095);writes++;
    if(adc_error_on_write)HAL_ADC_ErrorCallback(&hadc1);
    return fail_dac?HAL_ERROR:HAL_OK;
}
static int motor_on,motor_arms,motor_aborts;
static void MotorPWM_Power(int on,int arm){motor_on=on;if(arm)motor_arms++;}
static void MotorPWM_Abort(void){motor_on=0;motor_aborts++;}
static unsigned snapshot_records;
static void USB_Log_Event(const char *tag,const char *msg)
{if(!strncmp(tag,"PWR_",4)){assert(strlen(msg)<=48);snapshot_records++;}}
#include ".build/power_service.inc"
static void step(uint32_t now,int press,int fresh)
{tick=now;key=(uint8_t)(press?41:0);writes=0;if(fresh)HAL_ADC_ConvCpltCallback(&hadc1);Power_Service();}
int main(void)
{
    power_service_ready=1;
    step(10,0,0);assert(!enabled);
    step(20,1,0);assert(!enabled && power_control.fault==POWER_ADC_LOST);
    step(30,1,1);assert(power_control.state==POWER_OFF && !enabled);
    step(40,1,1);assert(enabled && power_control.state==POWER_STARTING && DAC_set==0);
    step(50,1,1);assert(!enabled && key==0);
    fail_dac=1;step(60,1,1);assert(!enabled && power_control.fault==POWER_DAC_ERROR);
    fail_dac=0;step(70,1,1);step(80,1,1);assert(enabled);
    step(180,0,0);assert(!enabled && power_control.fault==POWER_ADC_LOST);
    step(190,1,1);step(200,1,1);assert(enabled);
    adc_error_on_write=1;step(210,0,1);assert(!enabled && power_control.fault==POWER_ADC_LOST);
    assert(!motor_on && motor_aborts>0 && motor_arms==0);
    assert(snapshot_records==0);
    Power_LogService(999);assert(snapshot_records==0);
    for(unsigned i=0;i<9;i++)Power_LogService(1000+i*10);
    assert(snapshot_records==9);
    Power_LogService(1999);assert(snapshot_records==9);
    Power_LogService(2000);assert(snapshot_records==10);
    /* Existing supply button remains independent; motor local-arm key is
     * consumed only by service and only a healthy ON state grants power. */
    power_adc_error=0;power_control.state=POWER_ON;power_control.fault=0;
    power_control.target_v=12;power_control.target_i=1;
    adc_error_on_write=0;ADC_value[1]=21665;ADC_value[2]=2454;
    tick=3000;key=31;HAL_ADC_ConvCpltCallback(&hadc1);Power_Service();
    assert(enabled && motor_on && motor_arms==1 && key==0);
    step(3010,1,1);assert(!enabled && !motor_on && power_control.state==POWER_OFF);
    step(3020,1,1);assert(enabled && !motor_on && power_control.state==POWER_STARTING);
    step(3570,0,1);assert(enabled && motor_on && power_control.state==POWER_ON);
    assert(motor_arms==1); /* supply restart never requests rearming */
    puts("PASS actual Power_Service: DAC-before-enable, boot-off, stop, ADC freshness/error and DAC failure");
}
