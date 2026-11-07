#ifndef RT1064_REFERENCE_H
#define RT1064_REFERENCE_H
#include <stdint.h>
#include "rt1064_pid_reference.h"
/* Reference values only. Never sets vehicle_calibration.calibrated or starts PWM. */
#define RT1064_MOTOR_HZ 5000U
#define RT1064_SERVO_HZ 50U
#define RT1064_WHEEL_M 0.064f
#define RT1064_ENCODER_COUNTS 512.0f
#define RT1064_RATIO 2.7f
#define RT1064_PERIOD_S 0.01f
typedef struct {uint16_t duty_20000,duty_permille;uint8_t direction_high;} RT1064MotorReference;
float RT1064_MmPerCount(void);
float RT1064_CountsToMps(int32_t signed_delta);
float RT1064_TargetCounts(float speed_mps);
RT1064MotorReference RT1064_MotorCommand(int32_t pid_output);
uint16_t RT1064_ServoCorrect(int32_t command,uint16_t middle,uint16_t left,uint16_t right);
uint16_t RT1064_ServoAngle(float degrees);
void RT1064_InitBootPID(RT1064_PID *pid);
#endif
