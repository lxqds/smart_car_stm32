/* Actual app formatting + actual USB queue/chunker. Only HAL/USB endpoint is mocked. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "main.h"
#include "tim.h"
#include "power_log.h"
#include "usbd_cdc.h"
static uint32_t tick;
uint32_t HAL_GetTick(void){return tick;}
TIM_HandleTypeDef htim1;
int HAL_TIM_Encoder_Start(TIM_HandleTypeDef *t,uint32_t channel){(void)t;(void)channel;return HAL_OK;}
int VehiclePorts_Configure(uint32_t a,uint32_t b,uint32_t c){(void)a;(void)b;(void)c;assert(0);return 0;}
void VehiclePorts_Disable(void){}
void VehiclePorts_Apply(int16_t duty,uint16_t pulse){(void)duty;(void)pulse;}
int VCP_Desc,USBD_CDC;
static USBD_CDC_HandleTypeDef endpoint;
static uint8_t *pending;
static uint16_t pending_size;
static int busy_once;
static size_t wire_used;
static unsigned chunks;
static char wire[8192];
int USBD_Init(USBD_HandleTypeDef *d,void *desc,int n){(void)desc;(void)n;d->pClassData=&endpoint;return USBD_OK;}
int USBD_RegisterClass(USBD_HandleTypeDef *d,void *c){(void)d;(void)c;return USBD_OK;}
int USBD_Start(USBD_HandleTypeDef *d){d->dev_state=USBD_STATE_CONFIGURED;return USBD_OK;}
int USBD_CDC_RegisterInterface(USBD_HandleTypeDef *d,USBD_CDC_ItfTypeDef *i){(void)d;return i->init();}
int USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *d,uint8_t *p,uint16_t n){(void)d;pending=p;pending_size=n;return USBD_OK;}
int USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *d,uint8_t *p){(void)d;(void)p;return USBD_OK;}
int USBD_CDC_ReceivePacket(USBD_HandleTypeDef *d){(void)d;return USBD_OK;}
int USBD_CDC_TransmitPacket(USBD_HandleTypeDef *d)
{
    (void)d;assert(pending_size<=64);
    if(busy_once){busy_once=0;return USBD_BUSY;}
    assert(wire_used+pending_size<sizeof(wire));
    memcpy(wire+wire_used,pending,pending_size);wire_used+=pending_size;wire[wire_used]=0;chunks++;
    return USBD_OK;
}
#include "../USB_Log/usb_log.c"
static int led_level=-1;
#undef HAL_GPIO_WritePin
#define LED_GPIO_Port 3
#define LED_Pin GPIO_PIN_3
#define HAL_GPIO_WritePin(a,b,c) do {assert((a)==LED_GPIO_Port && (b)==LED_Pin);led_level=(c);} while(0)
#include "../Core/Src/vehicle_app.c"
static void drain(void)
{ unsigned guard=100;while(head!=tail && guard--)USB_Log_Poll();assert(guard>0); }
int main(void)
{
    USB_Log_Init();VehicleApp_Init();tick=0;drain();wire_used=0;wire[0]=0;
    tick=UINT32_MAX;last_status=tick;
    received=UINT32_MAX;received_servo=UINT16_MAX;
    uint32_t bits=0xffffffffU;memcpy(&received_speed,&bits,4);
    vehicle.encoder_delta=INT32_MIN;vehicle.faults=UINT32_MAX;vehicle.permit=UINT8_MAX;
    vehicle.encoder_raw_total=INT64_MIN;vehicle.encoder_changes=UINT32_MAX;
    vehicle.previous_count=65535;vehicle.encoder_diag_invalid=1;
    vehicle.speed_mps=-1;vehicle.distance_mm=-FLT_MAX;
    vehicle.motor_permille=INT16_MIN;vehicle.servo_us=UINT16_MAX;
    parser.bad=parser.unsupported=UINT32_MAX;
    VehicleApp_LogPoll();
    char expected[4096]="";unsigned records=0;
    for(unsigned i=tail;i!=head;i=(i+1)%16) {
        size_t n=strlen(queue[i]);assert(n<96 && n>=2 && !strcmp(queue[i]+n-2,"\r\n"));
        const char *detail=strchr(strchr(queue[i],' ')+1,' ')+1;
        assert(strlen(detail)-2<=48);
        strcat(expected,queue[i]);records++;
    }
    endpoint.TxState=1;USB_Log_Poll();assert(!wire_used);endpoint.TxState=0;
    busy_once=1;USB_Log_Poll();assert(!wire_used);
    chunks=0;drain();assert(!strcmp(wire,expected));assert(records==11 && chunks>records);
    const char *fields[]={"total=-9223372036854775808","cnt=65535 changes=4294967295 invalid=1","rx=4294967295","speed_bits=FFFFFFFF","servo=65535","delta=-2147483648",
        "fault=4294967295","permit=255","speed_mps_bits=BF800000","distance_mm_bits=FF7FFFFF",
        "duty=-32768","pulse=65535","bad=4294967295","unsupported=4294967295"};
    for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);i++) {
        if(!strstr(wire,fields[i]))fprintf(stderr,"Missing complete field: %s\n",fields[i]);
        assert(strstr(wire,fields[i]));
    }
    assert(strstr(wire,"ENC_EST_SPEED mps=-"));
    assert(strstr(wire,"ENC_EST_DIST m=-"));
    assert(!dropped);
    for(unsigned i=0;i<40;i++)USB_Log_Event("SATURATE","test");
    assert(dropped==25);drain();tick+=1000;USB_Log_Poll();drain();
    assert(strstr(wire,"SPEED_PI SERVO_ON dropped=25"));
    VehicleRxView view;VehicleApp_GetRxView(&view);assert(!view.seen);
    const uint8_t packet[]={0x24,0x42,1,11,0,0,0,0x3f,0xdc,5,0x92};
    tick=UINT32_MAX-50U;
    for(unsigned i=0;i<sizeof(packet);i++)VehicleApp_Receive(packet[i],tick);
    VehicleApp_GetRxView(&view);assert(view.seen && view.fresh && view.count==0);
    tick=148;VehicleApp_GetRxView(&view);assert(view.fresh && view.age_ms==199);
    tick=149;VehicleApp_GetRxView(&view);assert(!view.fresh && view.age_ms==200);
    for(unsigned i=0;i<sizeof(packet);i++)VehicleApp_Receive(packet[i],tick);
    VehicleApp_RxLost();VehicleApp_GetRxView(&view);assert(!view.fresh);
    /* Actual 10ms application path: active-low indicator, stale and disabled gates. */
    tick=1000;sample_tick=990;received_at=1000;received_seen=1;rx_lost=0;
    received_speed=0;received_servo=1500;
    parser.display_seen=1;parser.display_at=1000;parser.display_enabled=1;
    VehicleApp_Tick1ms();assert(led_level==GPIO_PIN_RESET);
    tick=1200;VehicleApp_Tick1ms();assert(led_level==GPIO_PIN_SET);
    tick=1210;received_at=1210;parser.display_at=1210;parser.display_enabled=0;
    VehicleApp_Tick1ms();assert(led_level==GPIO_PIN_SET);
    /* Real formatter -> real USB queue -> real 64-byte transport chunker. */
    drain();wire_used=0;wire[0]=0;
    PowerLogSnapshot ps={0};char pl[POWER_LOG_RECORDS][49];
    ps.sequence=ps.tick=ps.age=ps.fault=UINT32_MAX;ps.state=255;ps.enabled=1;
    ps.seen=ps.error=ps.valid=1;ps.raw[0]=ps.raw[1]=ps.raw[2]=65535;
    ps.set_v=ps.set_i=ps.in_v=ps.out_v=-1000000.0f;ps.out_i=-2.470f;
    ps.target_v=NAN;ps.target_i=INFINITY;ps.dac_v=3.3f;ps.dac_i=0;
    PowerLog_Format(&ps,pl);
    for(unsigned i=0;i<POWER_LOG_RECORDS;i++) {
        assert(strlen(pl[i])<=48);USB_Log_Event(PowerLog_Tag(i),pl[i]);
    }
    drain();
    for(unsigned i=0;i<POWER_LOG_RECORDS;i++)assert(strstr(wire,pl[i]));
    assert(strstr(wire,"out=-1000000.000"));assert(strstr(wire,"i=-2.470"));
    assert(strstr(wire,"u=invalid i=invalid"));
    assert(strstr(wire,"age=4294967295 seen=1 err=1 ok=1"));
    assert(strstr(wire,"sample_ms=4294967295"));
    puts("PASS power snapshot: actual 48-char USB path, integer maxima, negative values, nonfinite, 64-byte chunks");
    VehicleRxView health_view;
    uint8_t health[]={0x24,0x42,0x31,6,1,0x9e};
    Legacy_Reset(&parser);rx_lost=0;tick=UINT32_MAX-100U;
    for(unsigned i=0;i<6;i++)Legacy_Feed(&parser,health[i],tick,0,0);
    VehicleApp_GetRxView(&health_view);assert(health_view.gamepad_connected);
    tick=398;VehicleApp_GetRxView(&health_view);assert(health_view.gamepad_connected);
    tick=399;VehicleApp_GetRxView(&health_view);assert(!health_view.gamepad_connected);
    tick=400;for(unsigned i=0;i<6;i++)Legacy_Feed(&parser,health[i],tick,0,0);
    rx_lost=1;VehicleApp_GetRxView(&health_view);assert(!health_view.gamepad_connected);
    puts("PASS P2-3 actual vehicle formatting -> USB 48-char details/96-byte queue/64-byte chunks; maxima, negatives, busy retry, drop count");
}
