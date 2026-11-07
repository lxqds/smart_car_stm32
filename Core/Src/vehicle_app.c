#include "vehicle_app.h"
#include "vehicle_indicator.h"
#include "encoder_units.h"
#include "actuator_mode.h"
#include "vehicle_ports.h"
#include "vehicle_calibration.h"
#include "main.h"
#include "tim.h"
#include "usb_log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static VehicleCore vehicle;
static LegacyControlParser parser;
static volatile uint8_t ready;
static uint32_t sample_tick,log_tick,received;
static float received_speed;
static uint16_t received_servo;
static uint32_t received_at;
static uint8_t received_seen,rx_lost;
void VehicleApp_Init(void)
{
    ready=0;Legacy_Reset(&parser);
    Vehicle_Init(&vehicle,&vehicle_calibration,(uint16_t)__HAL_TIM_GET_COUNTER(&htim1));
    if(HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_ALL)!=HAL_OK)
        vehicle.faults|=VEHICLE_FAULT_ENCODER;
    if(VEHICLE_OUTPUTS_ALLOWED && !vehicle.faults &&
       !VehiclePorts_Configure(VEHICLE_TIMER_INPUT_HZ,VEHICLE_MOTOR_PWM_HZ,VEHICLE_SERVO_HZ))
        vehicle.faults|=VEHICLE_FAULT_CONFIG;
    sample_tick=HAL_GetTick();log_tick=sample_tick;
    /* Review gate is zero: no motor/servo PWM starts. Only TIM1 encoder starts. */
    __DMB();ready=1;
    USB_Log_Event("VEHICLE","LEGACY24_42 " ACTUATOR_MODE_TEXT);
    USB_Log_Event("VEHICLE_CAL","CALIBRATION_MISSING");
#if VEHICLE_LED_TEST_LOW
    USB_Log_Event("LED_TEST","PE3_LOW_FIXED POLARITY_UNKNOWN");
#endif
}
void VehicleApp_Tick1ms(void)
{
    if(!ready)return;
    uint32_t now=HAL_GetTick(),elapsed=now-sample_tick;
    if(elapsed<10U)return;
    sample_tick=now;
    Vehicle_Step10ms(&vehicle,(uint16_t)__HAL_TIM_GET_COUNTER(&htim1),now,elapsed);
    VehiclePorts_Apply(vehicle.motor_permille,vehicle.servo_us);
#if VEHICLE_LED_CONFIGURED
    VehicleRxView view;VehicleApp_GetRxView(&view);
    int enabled=view.gear_fresh && view.fresh && view.display_enabled;
    HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,
        enabled==VEHICLE_LED_ACTIVE_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}
void VehicleApp_SetPermit(int enabled)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    Vehicle_SetPermit(&vehicle,enabled && VEHICLE_OUTPUTS_ALLOWED);
    if(!vehicle.permit)VehiclePorts_Disable();
    __set_PRIMASK(mask);
}
static void command(void *unused,float speed,uint16_t servo,uint32_t arrived)
{
    (void)unused;
    uint32_t mask=__get_PRIMASK();__disable_irq();
    ++received;received_speed=speed;received_servo=servo;
    received_at=arrived;received_seen=1;rx_lost=0;
    Vehicle_Command(&vehicle,speed,servo,arrived,HAL_GetTick());
    __set_PRIMASK(mask);
}
void VehicleApp_Receive(uint8_t byte,uint32_t arrived)
{ if(ready)Legacy_Feed(&parser,byte,arrived,command,0); }
void VehicleApp_RxLost(void)
{
    parser.used=0;parser.display_seen=0;
    rx_lost=1;
    VehicleApp_SetPermit(0);
}
void VehicleApp_GetRxView(VehicleRxView *v)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    v->gear=parser.display_gear;
    v->gear_fresh=(uint8_t)(parser.display_seen && !rx_lost &&
        (uint32_t)(HAL_GetTick()-parser.display_at)<500U);
    v->display_enabled=(uint8_t)(v->gear_fresh && parser.display_enabled);
    v->gamepad_connected=(uint8_t)(parser.gamepad_seen && parser.gamepad_connected && !rx_lost &&
        (uint32_t)(HAL_GetTick()-parser.gamepad_at)<500U);
    v->count=received;v->received_mps=received_speed;v->received_pwm=received_servo;
    v->seen=received_seen;v->age_ms=HAL_GetTick()-received_at;v->faults=vehicle.faults;
    v->range_ok=(uint8_t)(isfinite(received_speed) && fabsf(received_speed)<=20 &&
                         received_servo>=500 && received_servo<=2500);
    v->fresh=(uint8_t)(v->seen && v->range_ok && !rx_lost && v->age_ms<vehicle.config.command_timeout_ms);
    __set_PRIMASK(mask);
}
void VehicleApp_GetEncoderView(VehicleEncoderView *v)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    v->delta=vehicle.encoder_delta;v->sampled_at=sample_tick;
    v->valid=(uint8_t)(ready && !vehicle.encoder_diag_invalid &&
        !(vehicle.faults & ~VEHICLE_FAULT_CONFIG));
    __set_PRIMASK(mask);
}
void VehicleApp_LogPoll(void)
{
    uint32_t now=HAL_GetTick();if(!ready || (uint32_t)(now-log_tick)<500U)return;
    log_tick=now;
    uint32_t mask=__get_PRIMASK();__disable_irq();
    VehicleCore copy=vehicle;uint32_t rx=received;float speed=received_speed;uint16_t servo=received_servo;
    __set_PRIMASK(mask);
    /* USB_Log_Event preserves only 48 detail chars. Each record is bounded
     * independently, including all integer extremes; do not grow one line. */
    char text[49];
    /* Hex float bits avoid unbounded/large out-of-range float-to-int casts. */
    uint32_t bits;memcpy(&bits,&speed,sizeof(bits));
    snprintf(text,sizeof(text),"fault=%lu permit=%u ALT_CORE_OFF",(unsigned long)copy.faults,copy.permit);
    USB_Log_Event("VEHICLE_SAFE",text);
    snprintf(text,sizeof(text),"rx=%lu servo=%u speed_bits=%08lX",(unsigned long)rx,servo,(unsigned long)bits);
    USB_Log_Event("VEHICLE_RX",text);
    snprintf(text,sizeof(text),"delta=%ld",(long)copy.encoder_delta);
    USB_Log_Event("VEHICLE_ENC",text);
    snprintf(text,sizeof(text),"total=%lld",(long long)copy.encoder_raw_total);
    USB_Log_Event("ENC_TOTAL",text);
    snprintf(text,sizeof(text),"cnt=%u changes=%lu invalid=%u",copy.previous_count,
             (unsigned long)copy.encoder_changes,copy.encoder_diag_invalid);
    USB_Log_Event("ENC_DIAG",text);
    /* Estimates only; invalid=1 in ENC_DIAG invalidates the sample series.
     * The active speed PI uses the same geometry via MotorCore_Feedback;
     * this does not replace physical wheel/gear calibration. */
    snprintf(text,sizeof(text),"mps=%.6f",(double)Encoder_EstSpeedMps(copy.encoder_delta,10));
    USB_Log_Event("ENC_EST_SPEED",text);
    snprintf(text,sizeof(text),"m=%.6f",(double)Encoder_EstDistanceM(copy.encoder_raw_total));
    USB_Log_Event("ENC_EST_DIST",text);
    uint32_t measured_bits,distance_bits;
    memcpy(&measured_bits,&copy.speed_mps,4);memcpy(&distance_bits,&copy.distance_mm,4);
    snprintf(text,sizeof(text),"speed_mps_bits=%08lX",(unsigned long)measured_bits);
    USB_Log_Event("VEHICLE_SPEED",text);
    snprintf(text,sizeof(text),"distance_mm_bits=%08lX",(unsigned long)distance_bits);
    USB_Log_Event("VEHICLE_ROUTE",text);
    snprintf(text,sizeof(text),"duty=%d pulse=%u",copy.motor_permille,copy.servo_us);
    USB_Log_Event("VEHICLE_PID_OUTPUT",text);
    snprintf(text,sizeof(text),"bad=%lu unsupported=%lu",(unsigned long)parser.bad,(unsigned long)parser.unsupported);
    USB_Log_Event("VEHICLE_RXERR",text);
}
