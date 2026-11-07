#include "vehicle_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static VehicleConfig config(void)
{ VehicleConfig c={1,2,100,20,0,1,1,0,1000,1500,2000,200,2000,200,1};return c; }
static void timeout_test(void)
{
    const uint32_t starts[]={1,UINT32_MAX-100U};
    const uint32_t ages[]={199,200,204};
    for(unsigned b=0;b<2;b++)for(unsigned a=0;a<3;a++)for(unsigned neutral=0;neutral<2;neutral++) {
        VehicleCore v;VehicleConfig c=config();uint32_t start=starts[b],now=start+ages[a];
        Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);
        assert(Vehicle_Command(&v,0,1500,start,start));
        assert(Vehicle_Command(&v,1,1500,start,start));
        int accepted=Vehicle_Command(&v,neutral?0:1,1500,now,now);
        if(ages[a]<200) {
            assert(accepted && v.permit && v.last_control_ms==now);
            Vehicle_Step10ms(&v,0,now+5,10);
            assert(neutral?v.motor_permille==0:v.motor_permille>0);
        }
        else {
            assert(!accepted && !v.permit && !v.have_control && !v.neutral_seen);
            assert(v.last_control_ms==start && v.target_mps==0 && v.integral==0);
            Vehicle_Step10ms(&v,0,now+5,10);assert(v.motor_permille==0 && v.servo_us==0);
            assert(!Vehicle_Command(&v,0,1500,now+6,now+6));
            Vehicle_SetPermit(&v,1);assert(!Vehicle_Command(&v,1,1500,now+7,now+7));
            assert(Vehicle_Command(&v,0,1500,now+8,now+8));
            assert(Vehicle_Command(&v,1,1500,now+9,now+9));
        }
    }
    VehicleCore v;VehicleConfig c=config();Vehicle_Init(&v,&c,0);Vehicle_SetPermit(&v,1);
    Vehicle_Step10ms(&v,0,1000,10);assert(v.permit && !v.have_control);
    assert(Vehicle_Command(&v,0,1500,1000,1000)); /* first neutral is not expired history */
    puts("PASS P2-1 before/exact/after deadline, local rearm+neutral, first permit, uint32 wrap");
}
static unsigned count,accepted;
static uint32_t stamps[4],processing_now;
static VehicleCore target;
static void capture(void *ctx,float speed,uint16_t pulse,uint32_t arrived)
{
    (void)ctx;assert(count<4);stamps[count++]=arrived;
    accepted+=(unsigned)Vehicle_Command(&target,speed,pulse,arrived,processing_now);
}
static void feed(LegacyControlParser *p,const uint8_t *data,unsigned n,uint32_t stamp)
{ for(unsigned i=0;i<n;i++)Legacy_Feed(p,data[i],stamp,capture,0); }
static void resync_test(void)
{
    /* Neutral frame sum: 24+42+01+0B+DC+05 = 0x53. */
    const uint8_t neutral[]={0x24,0x42,1,11,0,0,0,0,0xdc,5,0x53};
    const uint8_t outer[]={0x24,0x42,1,30};
    for(unsigned wrap=0;wrap<2;wrap++)for(unsigned packets=1;packets<=2;packets++)for(unsigned active=0;active<2;active++) {
        uint32_t base=wrap?UINT32_MAX-20U:0;
        LegacyControlParser p;Legacy_Reset(&p);VehicleConfig c=config();
        Vehicle_Init(&target,&c,0);Vehicle_SetPermit(&target,1);count=accepted=0;
        if(active)assert(Vehicle_Command(&target,0,1500,base-1U,base-1U));
        feed(&p,outer,4,base);feed(&p,neutral,11,base);
        if(packets==2)feed(&p,neutral,11,base+50);
        assert(count==0);
        unsigned remaining=30-4-11*packets;
        for(unsigned i=0;i<remaining;i++) {
            uint8_t padding=0;
            processing_now=base+50*(i+packets);
            feed(&p,&padding,1,processing_now);
        }
        assert(count==packets && stamps[0]==base);
        if(packets==2)assert(stamps[1]==base+50);
        assert(!accepted && !target.have_control && !target.permit);
        assert(target.last_control_ms==(active?base-1U:0));
        Vehicle_SetPermit(&target,1);processing_now+=1;
        feed(&p,neutral,11,processing_now);
        assert(count==packets+1 && stamps[packets]==processing_now && accepted==1);
    }
    puts("PASS P2-2 embedded old frames, delayed padding, multi-frame resync, uint32 wrap, stale rejection");
}
int main(int argc,char **argv)
{
    assert(argc==2);
    if(!strcmp(argv[1],"timeout"))timeout_test();
    else {assert(!strcmp(argv[1],"resync"));resync_test();}
}
