#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H
#include <stdint.h>
#include "actuator_mode.h"
/* RT1064 defaults are reference values, NOT measured mechanical limits. */
#define SERVO_CENTER_US 1500U
#define SERVO_LEFT_THRESHOLD 1140U
#define SERVO_RIGHT_THRESHOLD 2000U
#define SERVO_RISE_US 20U /* max change per 20ms, either direction: 1000us/s */
typedef struct {
    uint16_t command,pulse,target;
    uint8_t ready,armed,active;
    uint32_t reason;
} ServoView;
void Servo_Init(void);
void Servo_Power(int on,int local_arm);
void Servo_Tick(void);
void Servo_Abort(void);
void Servo_LogPoll(void);
void Servo_GetView(ServoView *v);
#endif
