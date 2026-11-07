#include "power_control.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    PowerControl p={0};
    Power_Update(&p,0,0,12,1,0,0,1);assert(p.state==POWER_OFF && p.dac_v==0);
    Power_Update(&p,1,1,12,1,0,0,1);assert(p.state==POWER_STARTING && p.dac_v==0);
    Power_Update(&p,276,0,30,10,0,0,1);
    assert(p.state==POWER_STARTING && p.target_v==12 && p.target_i==1);
    assert(fabsf(p.dac_v-12/(40.0f/4.3f+1)/2)<0.0001f);
    Power_Update(&p,551,0,12,1,12,0,1);assert(p.state==POWER_ON);
    Power_Update(&p,560,0,12,1,12,2,1);assert(p.state==POWER_FAULT && p.fault==POWER_OVERCURRENT && p.dac_v==0);
    Power_Update(&p,570,0,12,1,12,0,1);assert(p.state==POWER_FAULT);
    Power_Update(&p,580,1,12,1,12,0,1);assert(p.state==POWER_OFF);
    Power_Update(&p,590,1,12,1,0,0,1);assert(p.state==POWER_STARTING);
    Power_Update(&p,600,0,12,1,0,2,1);assert(p.fault==POWER_OVERCURRENT);
    Power_Update(&p,610,1,12,1,0,0,1);
    Power_Update(&p,620,1,NAN,1,0,0,1);assert(p.fault==POWER_BAD_SETTINGS);
    p=(PowerControl){0};Power_Update(&p,0,1,12,1,0,0,0);assert(p.fault==POWER_ADC_LOST);
    p=(PowerControl){0};Power_Update(&p,0xfffffff0U,1,12,1,0,0,1);
    Power_Update(&p,983,0,12,1,0,0,1);assert(p.state==POWER_STARTING);
    Power_Update(&p,984,0,12,1,0,0,1);assert(p.fault==POWER_NO_OUTPUT);
    p=(PowerControl){0};Power_Update(&p,0,1,12,1,0,0,1);
    Power_Update(&p,10,1,12,1,0,0,1);assert(p.state==POWER_OFF && p.dac_i==0);
    p=(PowerControl){0};Power_Update(&p,0,1,12,1,0,0,1);
    Power_Update(&p,10,0,12,1,0,0,0);assert(p.fault==POWER_ADC_LOST);
    puts("PASS power off/start/ramp/run/stop, startup overcurrent, fault latch/rearm, ADC loss, invalid input, startup timeout and wrap");
}
