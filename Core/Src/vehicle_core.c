#include "vehicle_core.h"
#include <math.h>
#include <string.h>
#include <limits.h>

static float bound(float x,float lo,float hi) { return x<lo?lo:(x>hi?hi:x); }
int Vehicle_ConfigValid(const VehicleConfig *c)
{
    return c->calibrated==1 && isfinite(c->mm_per_count) && c->mm_per_count>0 && c->mm_per_count<=100 &&
        isfinite(c->speed_limit_mps) && c->speed_limit_mps>0 && c->speed_limit_mps<=20 &&
        isfinite(c->kp) && c->kp>=0 && c->kp<=10000 &&
        isfinite(c->ki) && c->ki>=0 && c->ki<=10000 &&
        isfinite(c->kd) && c->kd>=0 && c->kd<=1000 &&
        (c->encoder_sign==1 || c->encoder_sign==-1) && (c->motor_sign==1 || c->motor_sign==-1) &&
        (c->servo_reverse==0 || c->servo_reverse==1) &&
        c->servo_min_us>=500 && c->servo_min_us<c->servo_center_us &&
        c->servo_center_us<c->servo_max_us && c->servo_max_us<=2500 &&
        c->duty_limit_permille>0 && c->duty_limit_permille<=1000 &&
        c->encoder_max_delta>0 && c->encoder_max_delta<32768 &&
        c->command_timeout_ms>=20 && c->command_timeout_ms<=1000;
}
void Vehicle_Stop(VehicleCore *v)
{
    v->target_mps=0;v->motor_permille=0;v->integral=0;v->last_measurement=v->speed_mps;
    v->servo_us=0;v->have_control=0;v->neutral_seen=0;
}
void Vehicle_Init(VehicleCore *v,const VehicleConfig *c,uint16_t counter)
{
    memset(v,0,sizeof(*v));v->config=*c;v->previous_count=counter;
    if(!Vehicle_ConfigValid(c))v->faults=VEHICLE_FAULT_CONFIG;
}
void Vehicle_SetPermit(VehicleCore *v,int permit)
{
    /* Repeated true is not a rearm: caller must first explicitly revoke. */
    if(!permit || !v->permit)Vehicle_Stop(v);
    v->permit=(uint8_t)(permit && !v->faults);
}
int Vehicle_Command(VehicleCore *v,float speed,uint16_t servo,uint32_t arrived,uint32_t now)
{
    const VehicleConfig *c=&v->config;
    /* Expire the existing authorization before a new packet can renew it.
     * A first local permit with no accepted control has no old deadline. */
    if(v->have_control && (uint32_t)(now-v->last_control_ms)>=c->command_timeout_ms) {
        Vehicle_Stop(v);v->permit=0;return 0;
    }
    if(!isfinite(speed) || fabsf(speed)>c->speed_limit_mps ||
       servo<c->servo_min_us || servo>c->servo_max_us ||
       (uint32_t)(now-arrived)>=c->command_timeout_ms) {Vehicle_Stop(v);v->permit=0;return 0;}
    if(!v->permit || v->faults)return 0;
    /* Legacy wire has no ARM/deadman/session. Local permit is mandatory. */
    if(!v->neutral_seen) {
        if(speed!=0 || servo!=c->servo_center_us)return 0;
        v->neutral_seen=1;
    }
    if((speed>0 && v->target_mps<0) || (speed<0 && v->target_mps>0))v->integral=0;
    v->target_mps=speed;v->command_servo_us=servo;
    v->last_control_ms=arrived;v->have_control=1;return 1;
}
void Vehicle_Step10ms(VehicleCore *v,uint16_t counter,uint32_t now,uint32_t elapsed)
{
    uint16_t raw=(uint16_t)(counter-v->previous_count);v->previous_count=counter;
    int32_t delta=raw<32768U?(int32_t)raw:(int32_t)raw-65536;
    v->encoder_delta=delta;
    /* Never grants control or changes calibrated distance. Sticky invalid flag
     * marks ambiguous half-wrap, missed cadence, or saturated diagnostics. */
    if(elapsed!=10U || raw==32768U)v->encoder_diag_invalid=1;
    if(delta) {
        if(v->encoder_changes==UINT32_MAX)v->encoder_diag_invalid=1;
        else v->encoder_changes++;
    }
    if((delta>0 && v->encoder_raw_total>INT64_MAX-delta) ||
       (delta<0 && v->encoder_raw_total<INT64_MIN-delta))v->encoder_diag_invalid=1;
    else v->encoder_raw_total+=delta;
    if(elapsed!=10U)v->faults|=VEHICLE_FAULT_TIMING;
    if(v->faults){Vehicle_Stop(v);return;}
    delta*=v->config.encoder_sign;v->encoder_delta=delta;
    if(delta>v->config.encoder_max_delta || delta<-(int32_t)v->config.encoder_max_delta) {
        v->faults|=VEHICLE_FAULT_ENCODER;Vehicle_Stop(v);return;
    }
    if((delta>0 && v->total_counts>INT64_MAX-delta) ||
       (delta<0 && v->total_counts<INT64_MIN-delta)) {
        v->faults|=VEHICLE_FAULT_NUMERIC;Vehicle_Stop(v);return;
    }
    v->total_counts+=delta;
    v->speed_mps=(float)delta*v->config.mm_per_count*0.1f; /* /10ms /1000 */
    v->distance_mm=(float)v->total_counts*v->config.mm_per_count;
    if(!isfinite(v->speed_mps) || !isfinite(v->distance_mm)) {
        v->faults|=VEHICLE_FAULT_NUMERIC;Vehicle_Stop(v);return;
    }
    if(!v->permit || !v->have_control) {Vehicle_Stop(v);return;}
    if((uint32_t)(now-v->last_control_ms)>=v->config.command_timeout_ms) {
        Vehicle_Stop(v);v->permit=0;return; /* timeout requires explicit local re-enable */
    }
    int pulse=v->command_servo_us;
    if(v->config.servo_reverse)pulse=2*(int)v->config.servo_center_us-pulse;
    v->servo_us=(uint16_t)bound((float)pulse,v->config.servo_min_us,v->config.servo_max_us);
    if(v->target_mps==0) {v->integral=0;v->last_measurement=v->speed_mps;v->motor_permille=0;return;}
    float error=v->target_mps-v->speed_mps;
    float limit=v->config.duty_limit_permille;
    float next_i=bound(v->integral+v->config.ki*error*0.01f,-limit,limit);
    float pd=v->config.kp*error-v->config.kd*(v->speed_mps-v->last_measurement)*100.0f;
    float candidate=pd+next_i;
    /* Conditional integration: allow unwinding but not further wind-up. */
    if((candidate<=limit && candidate>=-limit) ||
       (candidate>limit && error<0) || (candidate< -limit && error>0))v->integral=next_i;
    float output=pd+v->integral;
    if(!isfinite(output)) {v->faults|=VEHICLE_FAULT_NUMERIC;Vehicle_Stop(v);return;}
    v->last_measurement=v->speed_mps;
    v->motor_permille=(int16_t)(bound(output,-limit,limit)*v->config.motor_sign);
}
void Legacy_Reset(LegacyControlParser *p) { memset(p,0,sizeof(*p)); }
static void consume(LegacyControlParser *p,unsigned count)
{
    p->used=(uint8_t)(p->used-count);
    memmove(p->data,p->data+count,p->used);
    memmove(p->arrived,p->arrived+count,p->used*sizeof(p->arrived[0]));
}
void Legacy_Feed(LegacyControlParser *p,uint8_t b,uint32_t now,LegacyControlHandler handler,void *ctx)
{
    if(p->used && (uint32_t)(now-p->last_byte_ms)>=100U)p->used=0;
    p->last_byte_ms=now;p->data[p->used]=b;p->arrived[p->used]=now;p->used++;
    while(p->used) {
        if(p->data[0]!=0x24){consume(p,1);continue;}
        if(p->used<2)return;
        if(p->data[1]!=0x42){consume(p,1);continue;}
        if(p->used<4)return;
        unsigned length=p->data[3];
        if(length<5 || length>sizeof(p->data)){p->bad++;consume(p,1);continue;}
        if(p->used<length)return;
        uint8_t sum=0;for(unsigned i=0;i<length-1;i++)sum=(uint8_t)(sum+p->data[i]);
        if(sum!=p->data[length-1]){p->bad++;consume(p,1);continue;}
        if(p->data[2]==1 && length==11) {
            uint32_t bits=(uint32_t)p->data[4] | ((uint32_t)p->data[5]<<8) |
                ((uint32_t)p->data[6]<<16) | ((uint32_t)p->data[7]<<24);
            float speed;memcpy(&speed,&bits,4);
            uint16_t servo=(uint16_t)(p->data[8] | ((uint16_t)p->data[9]<<8));
            if(isfinite(speed)){p->good++;handler(ctx,speed,servo,p->arrived[length-1]);}else p->bad++;
        } else if(p->data[2]==0x30 && (length==7 || length==8)) {
            if(((length==7 && p->data[4]==1) ||
                (length==8 && p->data[4]==2 && p->data[6]<=1)) &&
               (p->data[5]>=1 || (length==8 && p->data[4]==2)) && p->data[5]<=7) {
                p->display_enabled=(uint8_t)(length==8 ? p->data[6] : 0);
                p->display_gear=p->data[5];p->display_seen=1;
                p->display_at=p->arrived[length-1];
            } else p->bad++;
        } else if(p->data[2]==0x31 && length==6) {
            /* Informational gamepad health; never grants actuator permission. */
            if(p->data[4]<=1) {
                p->gamepad_connected=p->data[4];p->gamepad_seen=1;
                p->gamepad_at=p->arrived[length-1];
            } else p->bad++;
        } else p->unsupported++;
        consume(p,length);
    }
}
