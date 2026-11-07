#include "vehicle_display.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
int main(void)
{
    char lines[4][41];VehicleRxView v={0};VehicleDisplay_Format(&v,lines);
    assert(strstr(lines[0],"WAIT"));
    v.seen=v.fresh=v.range_ok=1;v.received_mps=0.25f;v.received_pwm=1470;
    VehicleDisplay_Format(&v,lines);
    assert(strstr(lines[0],"FRESH") && strstr(lines[1],"250 mm/s") && strstr(lines[2],"10.0"));
    v.received_mps=-0.15f;v.received_pwm=1530;v.fresh=0;
    VehicleDisplay_Format(&v,lines);assert(strstr(lines[0],"STALE") && strstr(lines[1],"-150 mm/s") && strstr(lines[2],"-10.0"));
    v.gear=7;v.gear_fresh=1;v.fresh=1;v.display_enabled=1;VehicleDisplay_Format(&v,lines);
    assert(strstr(lines[0],"GEAR:7") && strstr(lines[0],"CTRL:ON"));
    v.gear=0;VehicleDisplay_Format(&v,lines);assert(strstr(lines[0],"GEAR:R"));
    v.gear_fresh=0;VehicleDisplay_Format(&v,lines);assert(strstr(lines[0],"GEAR:-"));
    v.count=v.faults=v.age_ms=UINT32_MAX;v.received_mps=20;v.received_pwm=2500;
    VehicleDisplay_Format(&v,lines);
    for(unsigned i=0;i<4;i++)assert(strlen(lines[i])<=40);
    assert(strstr(lines[3],"FFFFFFFF") && strstr(lines[3],"UNCAL"));
    v.received_mps=NAN;v.range_ok=0;VehicleDisplay_Format(&v,lines);
    assert(strstr(lines[0],"INVALID") && strstr(lines[1],"speed=--"));
    puts("PASS display WAIT/FRESH/STALE/INVALID, speed and inverse command-angle, 40-column bounds");
}
