#include <assert.h>
#include <stdio.h>
#include "tim.h"
#include "vehicle_ports.h"
#include "vehicle_app.h"
static TestTim t2,t5;
TIM_HandleTypeDef htim2={&t2,0,0,0},htim5={&t5,0,0,0};
static int starts,stops,initializations,pin,fail;
void MX_TIM2_Init(void){initializations++;}
void MX_TIM5_Init(void){assert(0);}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *t,int channel)
{assert(t==&htim2 && channel==TIM_CHANNEL_2);starts++;return fail;}
int HAL_TIM_PWM_Stop(TIM_HandleTypeDef *t,int channel)
{assert(t==&htim2 && channel==TIM_CHANNEL_2);stops++;return 0;}
void HAL_GPIO_WritePin(int port,int p,int value){(void)port;(void)p;pin=value;}
int main(void)
{
    if(!VEHICLE_OUTPUTS_ALLOWED) {
        assert(!VehiclePorts_Configure(240000000,5000,50));
        VehiclePorts_Apply(1000,1500);VehiclePorts_Disable();
        assert(!starts && !initializations && !htim2.ccr && !htim5.ccr);
        puts("PASS ports review gate: no timer init/start/output");return 0;
    }
    assert(!VehiclePorts_Configure(0,5000,50));assert(!VehiclePorts_Configure(240000000,0,50));
    assert(VehiclePorts_Configure(240000000,5000,50));
    assert(htim2.psc==0 && htim2.arr==47999 && htim5.arr==0 && starts==1);
    VehiclePorts_Apply(500,1500);assert(htim2.ccr==0 && pin==1 && htim5.ccr==0);
    VehiclePorts_Apply(500,1500);assert(htim2.ccr==24000);
    VehiclePorts_Apply(-500,1600);assert(htim2.ccr==0 && pin==1);
    VehiclePorts_Apply(-500,1600);assert(htim2.ccr==0 && pin==0);
    VehiclePorts_Apply(-500,1600);assert(htim2.ccr==24000);
    VehiclePorts_Apply(-2000,3000);assert(htim2.ccr==48000 && htim5.ccr==0);
    assert(!VehiclePorts_Configure(0,5000,50));assert(htim2.ccr==0 && htim5.ccr==0 && stops==1);
    fail=1;assert(!VehiclePorts_Configure(240000000,5000,50));assert(htim2.ccr==0 && htim5.ccr==0);
    puts("PASS simulated PWM frequency/limits/reversal blanking/reconfigure/start-failure");
}
