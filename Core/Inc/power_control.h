#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H
#include <stdint.h>
#include <math.h>
/* Software policy, not measured supply ratings. Retains legacy UI limits. */
typedef enum {POWER_OFF,POWER_STARTING,POWER_ON,POWER_FAULT} PowerState;
enum {POWER_BAD_SETTINGS=1,POWER_ADC_LOST=2,POWER_OVERCURRENT=3,
      POWER_NO_OUTPUT=4,POWER_DAC_ERROR=5};
typedef struct {
    PowerState state;
    uint32_t started;
    unsigned fault;
    float target_v,target_i,dac_v,dac_i;
} PowerControl;
static inline void Power_Fault(PowerControl *p,unsigned fault)
{p->state=POWER_FAULT;p->fault=fault;p->dac_v=p->dac_i=0;}
static inline void Power_Update(PowerControl *p,uint32_t now,int toggle,
                               float set_v,float set_i,float out_v,float out_i,int adc_valid)
{
    /* A fault-clear press never also starts the supply. */
    if(p->state==POWER_FAULT) {
        if(toggle){p->state=POWER_OFF;p->fault=0;}
        return;
    }
    if(toggle && p->state!=POWER_OFF) {
        p->state=POWER_OFF;p->dac_v=p->dac_i=0;return;
    }
    if(p->state==POWER_OFF && !toggle)return;
    if(!adc_valid || !isfinite(out_v) || !isfinite(out_i)) {
        Power_Fault(p,POWER_ADC_LOST);return;
    }
    if(p->state==POWER_OFF) {
        if(!isfinite(set_v)||!isfinite(set_i)||set_v<=0||set_v>30||set_i<=0||set_i>10) {
            Power_Fault(p,POWER_BAD_SETTINGS);return;
        }
        p->target_v=set_v;p->target_i=set_i;p->started=now;
        p->state=POWER_STARTING;
    }
    /* Overcurrent is active during startup too; startup does not mask it. */
    if(out_i>p->target_i+0.5f){Power_Fault(p,POWER_OVERCURRENT);return;}
    uint32_t elapsed=now-p->started;
    if(p->state==POWER_STARTING && elapsed>=550U && out_v>=0.001f)p->state=POWER_ON;
    if((p->state==POWER_ON || elapsed>=1000U) && out_v<0.001f) {
        Power_Fault(p,POWER_NO_OUTPUT);return;
    }
    float fraction=elapsed>=550U?1.0f:(float)elapsed/550.0f;
    if(p->state==POWER_ON)fraction=1;
    p->dac_v=p->target_v/(40.0f/4.3f+1.0f)*fraction;
    p->dac_i=p->target_i*0.005f*50.0f+0.6175f;
}
#endif
