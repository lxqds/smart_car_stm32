#ifndef VEHICLE_PORTS_H
#define VEHICLE_PORTS_H
#include <stdint.h>
/* Timer input clocks must be measured/derived from actual RCC configuration. */
int VehiclePorts_Configure(uint32_t timer_hz,uint32_t motor_pwm_hz,uint32_t servo_hz);
void VehiclePorts_Apply(int16_t duty_permille,uint16_t servo_us);
void VehiclePorts_Disable(void);
int MotorPort_Configure(uint32_t timer_hz,uint32_t motor_pwm_hz);
/* Motor-only normalized input: 0..20000; commissioning cap applied internally. */
void MotorPort_Apply(int16_t duty_20000);
uint32_t MotorPort_Compare(void);
uint32_t MotorPort_Period(void);
#endif
