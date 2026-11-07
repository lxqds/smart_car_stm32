#ifndef MOTOR_PWM_H
#define MOTOR_PWM_H
#include <stdint.h>
#include "actuator_mode.h"
/* Single motor owner: RT1064 count-domain speed PI, signed direction. */
#define MOTOR_PWM_COMMISSIONING (SERVO_BENCH_MODE != 1)
#define MOTOR_PWM_HZ 5000U /* RT1064 reference; verify with actual driver before power testing. */
#define MOTOR_PWM_FULL_SCALE 20000U /* RT1064-compatible normalized duty */
#define MOTOR_PWM_LIMIT 19500 /* RT1064: clamp 1950 then multiply by 10; 97.5% */
#define MOTOR_PWM_RISE_STEP 10 /* 5 percentage points/second */
#define MOTOR_SPEED_KP 8.0f
#define MOTOR_SPEED_KI 0.8f /* Per 10ms sample, legacy encoder-count units. */
#define MOTOR_SPEED_KD 0.0f
/* Engineering guard, not calibrated stall protection/current sensing. */
#define MOTOR_NO_MOTION_MS 1000U
#define MOTOR_NO_MOTION_DUTY 1000 /* 5% */
typedef struct {
    float target_mps, measured_mps;
    int16_t duty;
    uint8_t armed,running;
    uint32_t reason;
} MotorSpeedView;
void MotorPWM_GetView(MotorSpeedView *view);
void MotorPWM_Init(void);
void MotorPWM_Power(int on,int local_arm);
void MotorPWM_Tick(void);
void MotorPWM_Abort(void);
void MotorPWM_LogPoll(void);
#endif
