#ifndef VEHICLE_CALIBRATION_H
#define VEHICLE_CALIBRATION_H
#include "vehicle_core.h"
/* Pending hardware review. Zero clocks/PWM and calibrated=0 deliberately fail closed.
 * mm_per_count = wheel_circumference_mm / measured_quadrature_counts_per_wheel_turn.
 * Gains use error in m/s and output in signed permille, NOT RT1064 counts/10ms.
 */
#define VEHICLE_TIMER_INPUT_HZ 0U
#define VEHICLE_MOTOR_PWM_HZ 0U
#define VEHICLE_SERVO_HZ 0U
static const VehicleConfig vehicle_calibration={
    .mm_per_count=0.0f, .speed_limit_mps=0.5f,
    .kp=0.0f, .ki=0.0f, .kd=0.0f,
    .encoder_sign=1, .motor_sign=1, .servo_reverse=0,
    .servo_min_us=1000, .servo_center_us=1500, .servo_max_us=2000,
    .duty_limit_permille=200, .encoder_max_delta=1000,
    .command_timeout_ms=200, .calibrated=0
};
#endif
