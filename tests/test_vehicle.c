#include "encoder_units.h"
#include "vehicle_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned calls;
static float speed_received;
static uint16_t pulse_received;
static void receive(void *ctx,float speed,uint16_t pulse,uint32_t arrived)
{ (void)ctx;(void)arrived;calls++;speed_received=speed;pulse_received=pulse; }
static void packet(uint8_t *b,float speed,uint16_t pulse)
{
    b[0]=0x24;b[1]=0x42;b[2]=1;b[3]=11;
    uint32_t bits;memcpy(&bits,&speed,4);
    for(unsigned i=0;i<4;i++)b[4+i]=(uint8_t)(bits>>(8*i));
    b[8]=(uint8_t)pulse;b[9]=(uint8_t)(pulse>>8);b[10]=0;
    for(unsigned i=0;i<10;i++)b[10]=(uint8_t)(b[10]+b[i]);
}
static void bytes(LegacyControlParser *p,const uint8_t *b,unsigned n,uint32_t tick)
{ for(unsigned i=0;i<n;i++){Legacy_Feed(p,b[i],tick,receive,0);assert(p->used<30);} }
static VehicleConfig config(void)
{
    VehicleConfig c={1.0f,2.0f,100.0f,20.0f,0.0f,1,1,0,1000,1500,2000,200,2000,200,1};
    return c;
}
int main(void)
{
    LegacyControlParser p;uint8_t b[11];packet(b,0.5f,1500);
    const uint8_t golden[]={0x24,0x42,1,11,0,0,0,0x3f,0xdc,5,0x92};
    assert(!memcmp(b,golden,11));
    for(unsigned split=0;split<=11;split++) {
        Legacy_Reset(&p);calls=0;bytes(&p,b,split,0);bytes(&p,b+split,11-split,1);
        assert(calls==1 && speed_received==0.5f && pulse_received==1500);
    }
    Legacy_Reset(&p);calls=0;for(unsigned i=0;i<1000;i++)bytes(&p,b,11,i);assert(calls==1000);
    for(unsigned i=0;i<11;i++) {
        uint8_t bad[11];memcpy(bad,b,11);bad[i]^=0x80;
        Legacy_Reset(&p);calls=0;bytes(&p,bad,11,0);bytes(&p,b,11,1);bytes(&p,b,11,2);assert(calls>=1);
    }
    Legacy_Reset(&p);calls=0;packet(b,NAN,1500);bytes(&p,b,11,0);assert(!calls && p.bad==1);
    packet(b,-1.0f,1600);bytes(&p,b,5,UINT32_MAX-50U);bytes(&p,b,11,49);assert(calls==1);
    uint32_t rng=1;for(unsigned i=0;i<100000;i++){rng=rng*1664525U+1013904223U;uint8_t x=(uint8_t)(rng>>24);bytes(&p,&x,1,50);}
    VehicleConfig c=config();VehicleCore v;assert(Vehicle_ConfigValid(&c));
    Vehicle_Init(&v,&c,65530);Vehicle_Step10ms(&v,4,10,10);
    assert(v.encoder_delta==10 && v.total_counts==10 && fabsf(v.speed_mps-1)<0.001f);
    Vehicle_Step10ms(&v,65530,20,10);assert(v.encoder_delta==-10 && v.total_counts==0);
    assert(!Vehicle_Command(&v,0,1500,20,20)); /* no local permission */
    Vehicle_SetPermit(&v,1);assert(!Vehicle_Command(&v,1,1500,20,20));
    assert(Vehicle_Command(&v,0,1500,20,20));assert(Vehicle_Command(&v,1,1600,21,21));
    Vehicle_Step10ms(&v,65530,30,10);assert(v.motor_permille>0 && v.servo_us==1600);
    Vehicle_Step10ms(&v,65530,221,10);assert(!v.permit && !v.motor_permille && !v.servo_us);
    assert(!Vehicle_Command(&v,0,1500,222,222));
    Vehicle_SetPermit(&v,1);assert(Vehicle_Command(&v,0,1500,223,223));
    assert(!Vehicle_Command(&v,NAN,1500,224,224) && !v.permit);
    Vehicle_SetPermit(&v,1);assert(!Vehicle_Command(&v,0,1500,0,200));
    Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);
    assert(Vehicle_Command(&v,0,1500,UINT32_MAX-10U,UINT32_MAX-10U));
    Vehicle_Step10ms(&v,0,5,10);assert(v.permit);
    Vehicle_Step10ms(&v,0,189,10);assert(!v.permit);
    Vehicle_Init(&v,&c,0);Vehicle_Step10ms(&v,32768,10,10);assert(v.faults&VEHICLE_FAULT_ENCODER);
    Vehicle_Init(&v,&c,0);Vehicle_Step10ms(&v,0,20,20);assert(v.faults&VEHICLE_FAULT_TIMING);
    c.calibrated=0;Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);assert(!v.permit && v.faults);
    c=config();c.kp=1000;c.ki=100;Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);
    assert(Vehicle_Command(&v,0,1500,0,0));
    for(unsigned i=1;i<=1000;i++) {
        assert(Vehicle_Command(&v,2,1500,i*10,i*10));Vehicle_Step10ms(&v,0,i*10,10);
        assert(v.motor_permille==200 && v.integral==0);
    }
    assert(Vehicle_Command(&v,0.1f,1500,10010,10010));Vehicle_Step10ms(&v,0,10010,10);
    assert(v.motor_permille>=100 && v.motor_permille<105); /* recovers from saturation */
    assert(Vehicle_Command(&v,0,1500,10020,10020));Vehicle_Step10ms(&v,0,10020,10);
    assert(v.motor_permille==0 && v.integral==0);
    c=config();c.encoder_sign=-1;c.motor_sign=-1;c.servo_reverse=1;
    Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);assert(Vehicle_Command(&v,0,1500,0,0));
    assert(Vehicle_Command(&v,1,1200,0,0));Vehicle_Step10ms(&v,1,10,10);
    assert(v.encoder_delta==-1 && v.motor_permille<0 && v.servo_us==1800);
    c=config();c.calibrated=0;Vehicle_Init(&v,&c,65530);
    Vehicle_Step10ms(&v,4,10,10);
    assert(v.encoder_raw_total==10 && v.encoder_changes==1 && !v.encoder_diag_invalid);
    Vehicle_Step10ms(&v,65530,20,10);
    assert(v.encoder_raw_total==0 && v.encoder_changes==2);
    for(unsigned i=0;i<1000;i++)Vehicle_Step10ms(&v,65530,30+i*10,10);
    assert(v.encoder_raw_total==0 && v.encoder_changes==2 && !v.encoder_diag_invalid);
    assert(!v.total_counts && !v.motor_permille && !v.servo_us && !v.permit);
    Vehicle_Stop(&v);assert(v.encoder_changes==2);
    v.encoder_raw_total=INT64_MAX;Vehicle_Step10ms(&v,65531,0,10);
    assert(v.encoder_raw_total==INT64_MAX && v.encoder_diag_invalid);
    Vehicle_Init(&v,&c,0);v.encoder_raw_total=INT64_MIN;
    Vehicle_Step10ms(&v,65535,10,10);assert(v.encoder_raw_total==INT64_MIN && v.encoder_diag_invalid);
    Vehicle_Init(&v,&c,0);Vehicle_Step10ms(&v,32768,10,10);assert(v.encoder_diag_invalid);
    Vehicle_Init(&v,&c,0);Vehicle_Step10ms(&v,0,20,20);assert(v.encoder_diag_invalid);
    puts("PASS raw encoder diagnostic: uncalibrated, stationary, bidirectional wrap, stop, overflow, cadence");
    assert(fabsf(ENCODER_STM32_COUNTS_PER_TURN-5529.6f)<0.001f);
    assert(fabsf(Encoder_EstSpeedMps(400,10)-1.454441f)<0.00001f);
    assert(fabsf(Encoder_EstSpeedMps(-400,20)+0.7272205f)<0.00001f);
    assert(Encoder_EstSpeedMps(1,10)>0); /* no integer /4 truncation */
    assert(Encoder_EstSpeedMps(1,0)==0);
    assert(fabsf(Encoder_EstDistanceM(400)-0.01454441f)<0.000001f);
    puts("PASS encoder units: legacy x1/STM32 x4 equivalence, signs, interval, small increments");
    puts("PASS legacy 24 42 parser; encoder wrap/sign; PID saturation/recovery; freshness/permit/calibration gates");
}
