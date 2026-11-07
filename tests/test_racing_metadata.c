#include "vehicle_core.h"
#include <assert.h>
#include <stdio.h>
static int controls;
static void command(void *ctx,float s,uint16_t pwm,uint32_t at)
{(void)ctx;(void)s;(void)pwm;(void)at;controls++;}
static void feed(LegacyControlParser *p,uint8_t gear,uint8_t version,uint32_t at,int corrupt)
{
    uint8_t b[]={0x24,0x42,0x30,7,version,gear,0};
    for(unsigned i=0;i<6;i++)b[6]=(uint8_t)(b[6]+b[i]);
    b[6]^=(uint8_t)corrupt;
    for(unsigned i=0;i<7;i++)Legacy_Feed(p,b[i],at,command,0);
}
int main(void)
{
    LegacyControlParser p;Legacy_Reset(&p);
    feed(&p,7,1,0xfffffff0U,0);
    assert(p.display_seen && p.display_gear==7 && p.display_at==0xfffffff0U);
    feed(&p,0,1,20,0);feed(&p,8,1,21,0);feed(&p,3,2,22,0);feed(&p,3,1,23,1);
    assert(p.display_gear==7 && p.display_at==0xfffffff0U && controls==0 && p.good==0);
    feed(&p,1,1,24,0);assert(p.display_gear==1 && p.display_at==24 && controls==0);
    uint8_t v2[]={0x24,0x42,0x30,8,2,7,1,0};
    for(unsigned i=0;i<7;i++)v2[7]=(uint8_t)(v2[7]+v2[i]);
    for(unsigned i=0;i<8;i++)Legacy_Feed(&p,v2[i],30,command,0);
    assert(p.display_enabled==1 && p.display_gear==7 && p.display_at==30 && controls==0);
    v2[6]=2;v2[7]++;
    for(unsigned i=0;i<8;i++)Legacy_Feed(&p,v2[i],31,command,0);
    assert(p.display_at==30); /* invalid flags cannot refresh display */
    feed(&p,1,1,32,0);assert(p.display_enabled==0);
    uint8_t reverse[]={0x24,0x42,0x30,8,2,0,1,0};
    for(unsigned i=0;i<7;i++)reverse[7]=(uint8_t)(reverse[7]+reverse[i]);
    for(unsigned i=0;i<8;i++)Legacy_Feed(&p,reverse[i],33,command,0);
    assert(p.display_gear==0 && p.display_enabled && p.display_at==33 && controls==0);
    uint8_t health[]={0x24,0x42,0x31,6,1,0x9e};
    for(unsigned i=0;i<6;i++)Legacy_Feed(&p,health[i],40,command,0);
    assert(p.gamepad_seen && p.gamepad_connected && p.gamepad_at==40 && controls==0);
    health[4]=2;health[5]++;
    for(unsigned i=0;i<6;i++)Legacy_Feed(&p,health[i],41,command,0);
    assert(p.gamepad_at==40); /* invalid state cannot refresh */
    health[4]=0;health[5]=0x9d;
    for(unsigned i=0;i<6;i++)Legacy_Feed(&p,health[i],42,command,0);
    assert(!p.gamepad_connected && p.gamepad_at==42 && controls==0);
    puts("PASS display metadata range/version/checksum/recovery/wrap; no control callback");
}
