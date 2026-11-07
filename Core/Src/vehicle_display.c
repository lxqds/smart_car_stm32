#include "vehicle_display.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifndef VEHICLE_DISPLAY_HOST_TEST
#include "lcd_o.h"
#include "usb_log.h"
#include "motor_pwm.h"
#include "servo_control.h"
#endif
void VehicleDisplay_Format(const VehicleRxView *v,char lines[4][41])
{
    const char *state=!v->seen?"WAIT":(!v->range_ok?"INVALID":(v->fresh?"FRESH":"STALE"));
    snprintf(lines[0],41,"RX %-7s GEAR:%c CTRL:%s",state,
             v->gear_fresh && v->gear<=7 ? (v->gear ? '0'+v->gear : 'R') : '-',
             v->gear_fresh && v->fresh && v->display_enabled ? "ON" : "OFF");
    if(v->seen && v->range_ok && isfinite(v->received_mps) && fabsf(v->received_mps)<=20) {
        long speed=(long)roundf(v->received_mps*1000);
        /* Exact inverse may be fractional after old host integer truncation. */
        int angle10=(1500-(int)v->received_pwm)*10/3;
        snprintf(lines[1],41,"CMD speed=%ld mm/s PWM=%u",speed,v->received_pwm);
        snprintf(lines[2],41,"CMD angle=%s%d.%d K=3 (not measured)",angle10<0?"-":"",
                 abs(angle10)/10,abs(angle10)%10);
    } else {
        snprintf(lines[1],41,"CMD speed=-- PWM=%u",v->received_pwm);
        snprintf(lines[2],41,"CMD angle=-- K=3 (not measured)");
    }
    snprintf(lines[3],41,"age=%lums F=%08lX ENC:UNCAL",
             (unsigned long)(v->age_ms>9999?9999:v->age_ms),(unsigned long)v->faults);
}
#ifndef VEHICLE_DISPLAY_HOST_TEST
void VehicleDisplay_Poll(void)
{
    static uint32_t last;
    static char previous[4][41];
    uint32_t now=HAL_GetTick();if((uint32_t)(now-last)<200)return;last=now;
    if(lcddev.width<320 || lcddev.height<232)return;
    VehicleRxView v;char lines[4][41];VehicleApp_GetRxView(&v);VehicleDisplay_Format(&v,lines);
#if ACTUATORS_COMBINED
    ServoView servo;Servo_GetView(&servo);
    MotorSpeedView motor;MotorPWM_GetView(&motor);
    snprintf(lines[3],41,"V%ld S%u A%u/%u F%lu/%lu",
             (long)(motor.measured_mps*1000),servo.pulse,motor.armed,servo.armed,
             (unsigned long)motor.reason,(unsigned long)servo.reason);
#elif SERVO_BENCH_MODE
    ServoView servo;Servo_GetView(&servo);
    snprintf(lines[3],41,"SERVO:%uus A%u ON%u STOP%lu MOTOR_OFF",
             servo.pulse,servo.armed,servo.active,(unsigned long)servo.reason);
#else
    MotorSpeedView motor;MotorPWM_GetView(&motor);
    /* Estimated wheel speed; original power UI and steering command remain. */
    snprintf(lines[3],41,"V~%ld U:%d.%d%% A%u R%u STOP%lu",
             (long)(motor.measured_mps*1000),motor.duty/200,(motor.duty%200)/20,
             motor.armed,motor.running,(unsigned long)motor.reason);
#endif
    uint16_t foreground=POINT_COLOR,background=BACK_COLOR;
    POINT_COLOR=WHITE;BACK_COLOR=BLUE;
    for(unsigned i=0;i<4;i++)if(strcmp(previous[i],lines[i])) {
        /* 40 characters at 8x16; pad instead of full-screen clears. */
        char padded[41];memset(padded,' ',40);padded[40]=0;
        memcpy(padded,lines[i],strlen(lines[i]));
        LCD_ShowString(0,(uint16_t)(168+16*i),320,16,16,(uint8_t *)padded);
        strcpy(previous[i],lines[i]);
    }
    POINT_COLOR=foreground;BACK_COLOR=background;
    USB_Log_Event("SCREEN_STATE",lines[0]);
    USB_Log_Event("SCREEN_TARGET",lines[1]);
    USB_Log_Event("SCREEN_ANGLE",lines[2]);
}
#endif
