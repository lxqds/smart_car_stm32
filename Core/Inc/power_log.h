#ifndef POWER_LOG_H
#define POWER_LOG_H
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#define POWER_LOG_RECORDS 9
typedef struct {
    uint32_t sequence,tick,age,fault;
    unsigned state,enabled,seen,error,valid;
    uint16_t raw[3];
    float set_v,set_i,in_v,out_v,out_i,target_v,target_i,dac_v,dac_i;
} PowerLogSnapshot;
static inline void PowerLog_Number(char out[13],float value)
{
    if(!isfinite(value) || fabsf(value)>1000000.0f)snprintf(out,13,"invalid");
    else snprintf(out,13,"%.3f",(double)value);
}
static inline const char *PowerLog_Tag(unsigned n)
{
    static const char *const tags[POWER_LOG_RECORDS]={"PWR_STATE","PWR_SET","PWR_VOLT",
        "PWR_CURR","PWR_TARGET","PWR_DAC","PWR_ADC","PWR_RAW","PWR_TIME"};
    return tags[n];
}
/* Each detail fits USB_Log_Event's 48-char field, including uint32 sequence wrap. */
static inline void PowerLog_Format(const PowerLogSnapshot *s,char lines[POWER_LOG_RECORDS][49])
{
    unsigned long seq=(unsigned long)s->sequence;
    char a[13],b[13];
    snprintf(lines[0],49,"s=%lu st=%u fault=%lu en=%u",seq,
             s->state<=3?s->state:255,(unsigned long)s->fault,!!s->enabled);
    PowerLog_Number(a,s->set_v);PowerLog_Number(b,s->set_i);
    snprintf(lines[1],49,"s=%lu u=%s i=%s",seq,a,b);
    PowerLog_Number(a,s->in_v);PowerLog_Number(b,s->out_v);
    snprintf(lines[2],49,"s=%lu in=%s out=%s",seq,a,b);
    PowerLog_Number(a,s->out_i);snprintf(lines[3],49,"s=%lu i=%s",seq,a);
    PowerLog_Number(a,s->target_v);PowerLog_Number(b,s->target_i);
    snprintf(lines[4],49,"s=%lu u=%s i=%s",seq,a,b);
    PowerLog_Number(a,s->dac_v);PowerLog_Number(b,s->dac_i);
    snprintf(lines[5],49,"s=%lu u=%s i=%s",seq,a,b);
    snprintf(lines[6],49,"s=%lu age=%lu seen=%u err=%u ok=%u",seq,
             (unsigned long)s->age,!!s->seen,!!s->error,!!s->valid);
    snprintf(lines[7],49,"s=%lu a=%u b=%u c=%u",seq,s->raw[0],s->raw[1],s->raw[2]);
    snprintf(lines[8],49,"s=%lu sample_ms=%lu",seq,(unsigned long)s->tick);
}
#endif
