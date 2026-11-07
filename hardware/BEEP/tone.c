#include "tone.h"
#include "tim.h"
#include "vehicle_app.h"
#include "motor_pwm.h"
#include "servo_control.h"

/* PD11/TIM13 output; TIM13 is configured for a 1MHz tick. */
#define TONE_FREQ_MAX 500000U
#define CONTROL_TONE_HZ 900U
static volatile uint32_t toggles;
static volatile uint8_t continuous;
static uint8_t control_ready,control_enabled,remaining,sounding;
static uint32_t phase_at,interval_ms,poll_at;
static uint8_t gamepad_connected,alarm_active,pending_connection;
static uint32_t motor_reason,servo_reason;
static void silence(void)
{
    HAL_TIM_Base_Stop_IT(&htim13);
    __HAL_TIM_CLEAR_FLAG(&htim13,TIM_FLAG_UPDATE);
    HAL_GPIO_WritePin(BEEP_GPIO_Port,BEEP_Pin,GPIO_PIN_RESET);
    toggles=0;continuous=0;
}
static void start_tone(uint32_t freq,uint32_t duration)
{
    silence();
    if(!freq || freq>TONE_FREQ_MAX || !duration)return;
    uint64_t count=(uint64_t)freq*duration*2U/1000U;
    continuous=(uint8_t)(duration==TONE_DURATION_INFINITE);
    if(!continuous && (!count || count>UINT32_MAX))return;
    toggles=continuous?0:(uint32_t)count;
    __HAL_TIM_SET_AUTORELOAD(&htim13,TONE_FREQ_MAX/freq-1U);
    __HAL_TIM_SET_COUNTER(&htim13,0);
    htim13.Instance->EGR=TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim13,TIM_FLAG_UPDATE);
    if(HAL_TIM_Base_Start_IT(&htim13)!=HAL_OK)silence();
}
void Tone_TimerHandler(void)
{
    if(!continuous && !toggles){silence();return;}
    HAL_GPIO_TogglePin(BEEP_GPIO_Port,BEEP_Pin);
    if(!continuous && --toggles==0)silence();
}
void tone(uint32_t freq,uint32_t duration)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    /* Preserve UI clicks, but do not interrupt control-state patterns. */
    if(!remaining)start_tone(freq,duration);
    __set_PRIMASK(mask);
}
void noTone(void)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    remaining=sounding=alarm_active=pending_connection=0;silence();__set_PRIMASK(mask);
}
void Tone_ControlInit(void)
{
    uint32_t mask=__get_PRIMASK();__disable_irq();
    silence();control_enabled=remaining=sounding=0;
    MotorSpeedView m;ServoView s;MotorPWM_GetView(&m);Servo_GetView(&s);
    motor_reason=m.reason;servo_reason=s.reason;gamepad_connected=alarm_active=pending_connection=0;
    poll_at=HAL_GetTick();control_ready=1;__set_PRIMASK(mask);
}
void Tone_ControlPoll(void)
{
    if(!control_ready)return;
    uint32_t now=HAL_GetTick();if((uint32_t)(now-poll_at)<10U)return;
    poll_at=now;
    VehicleRxView v;VehicleApp_GetRxView(&v);
    MotorSpeedView m;ServoView s;MotorPWM_GetView(&m);Servo_GetView(&s);
    uint8_t enabled=(uint8_t)(v.fresh && v.gear_fresh && v.display_enabled);
    /* Ignore boot/power-off/Y-off codes; power lease loss while previously
     * authorized is identified by the previous zero (armed/ready) reason. */
    int motor_fault=(m.reason!=motor_reason && m.reason!=0 && m.reason!=3 &&
                     (m.reason!=1 || motor_reason==0));
    int servo_fault=(s.reason!=servo_reason && s.reason!=0 && s.reason!=3 &&
                     (s.reason!=1 || servo_reason==0));
    /* Linked shutdown may give the peer reason=7 after a normal Y stop. */
    int normal_off=!enabled && ((m.reason==3 && motor_reason!=3) ||
                               (s.reason==3 && servo_reason!=3));
    int link_lost=control_enabled && (!v.fresh || !v.gear_fresh ||
                                     (gamepad_connected && !v.gamepad_connected));
    int warning=((motor_fault || servo_fault) && !normal_off) || link_lost;
    int connection=v.gamepad_connected && !gamepad_connected;
    uint32_t mask=__get_PRIMASK();__disable_irq();
    int changed=enabled!=control_enabled;
    if(!v.gamepad_connected)pending_connection=0;
    else if(connection)pending_connection=1;
    control_enabled=enabled;gamepad_connected=v.gamepad_connected;
    motor_reason=m.reason;servo_reason=s.reason;
    if(warning && !alarm_active) {
        alarm_active=1;remaining=sounding=1;interval_ms=500;phase_at=now;
        start_tone(CONTROL_TONE_HZ,500);
    } else if(alarm_active) {
        if((uint32_t)(now-phase_at)>=500U){silence();remaining=sounding=alarm_active=0;}
    } else if(changed) {
        /* RT1064 SysStart: 60ms x6 transitions; Ok: 70ms x4.
         * New state replaces the old pattern; boot OFF stays silent. */
        remaining=enabled?3:2;interval_ms=enabled?60U:70U;
        sounding=1;phase_at=now;start_tone(CONTROL_TONE_HZ,interval_ms);
    } else if(pending_connection && !remaining) {
        pending_connection=0;
        remaining=sounding=1;interval_ms=70;phase_at=now;
        start_tone(CONTROL_TONE_HZ,70);
    } else if(remaining && (uint32_t)(now-phase_at)>=interval_ms) {
        phase_at=now;
        if(sounding){silence();sounding=0;--remaining;}
        else {sounding=1;start_tone(CONTROL_TONE_HZ,interval_ms);}
    }
    __set_PRIMASK(mask);
}
