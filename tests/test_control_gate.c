#include <assert.h>
#include <stdio.h>
#include "../framework/control_gate.h"
int main(void)
{
    ControlGate g; ControlLimits limits={200,500,15000,0};
    ControlIntent in={1,0,0,1,1};
    ControlGate_Init(&g,limits);assert(!ControlGate_Submit(&g,&in,0));
    ControlGate_SetLink(&g,1);assert(ControlGate_Submit(&g,&in,0));assert(g.state==CONTROL_ARMED);
    in.sequence++;in.speed_mm_s=100;in.arm_request=0;
    assert(ControlGate_Submit(&g,&in,20));assert(g.speed_mm_s==0); /* hard output gate */
    g.limits.allow_outputs=1; /* host simulation only, no hardware API exists */
    in.sequence++;assert(ControlGate_Submit(&g,&in,40));assert(g.speed_mm_s==100);
    assert(!ControlGate_Submit(&g,&in,100));
    ControlGate_Tick(&g,240);assert(g.state==CONTROL_DISARMED&&g.speed_mm_s==0);
    in.sequence++;assert(ControlGate_Submit(&g,&in,241));assert(g.state==CONTROL_DISARMED);
    in.sequence++;in.speed_mm_s=0;in.arm_request=1;assert(ControlGate_Submit(&g,&in,250));
    in.sequence++;in.deadman=0;assert(ControlGate_Submit(&g,&in,260));assert(g.state==CONTROL_DISARMED);
    ControlGate_SetLink(&g,0);ControlGate_SetLink(&g,1);assert(g.state==CONTROL_DISARMED);
    in=(ControlIntent){UINT32_MAX,0,0,1,1};assert(ControlGate_Submit(&g,&in,UINT32_MAX-10U));
    in.sequence=0;in.speed_mm_s=123;in.arm_request=0;assert(ControlGate_Submit(&g,&in,5));
    ControlGate_Tick(&g,205);assert(g.state==CONTROL_DISARMED);
    ControlGate_Estop(&g);ControlGate_SetLink(&g,1);assert(!ControlGate_Submit(&g,&in,206));
    assert(g.state==CONTROL_FAULT&&g.speed_mm_s==0);
    puts("PASS draft control gate: arming, deadman, stale/duplicate, reconnect, wrap, estop, outputs disabled");
}
