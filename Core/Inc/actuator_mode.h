#ifndef ACTUATOR_MODE_H
#define ACTUATOR_MODE_H
/* 0: speed only; 1: servo only; 2: integrated speed + steering.
 * Keep isolated modes available for regression and hardware diagnosis. */
#ifndef SERVO_BENCH_MODE
#define SERVO_BENCH_MODE 2
#endif
#if SERVO_BENCH_MODE < 0 || SERVO_BENCH_MODE > 2
#error Invalid actuator mode
#endif
#define ACTUATORS_COMBINED (SERVO_BENCH_MODE == 2)
#define SERVO_OUTPUT_ENABLED (SERVO_BENCH_MODE != 0)
#if ACTUATORS_COMBINED
#define ACTUATOR_MODE_TEXT "SPEED_PI SERVO_ON"
#elif SERVO_BENCH_MODE
#define ACTUATOR_MODE_TEXT "SERVO_ONLY MOTOR_OFF"
#else
#define ACTUATOR_MODE_TEXT "SPEED_PI SERVO_OFF"
#endif
#endif
