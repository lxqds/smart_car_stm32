#ifndef VEHICLE_APP_H
#define VEHICLE_APP_H
#include <stdint.h>
#include "vehicle_core.h"
/* REVIEW BUILD ONLY. No remote packet can override this gate. */
#ifndef VEHICLE_OUTPUTS_ALLOWED
#define VEHICLE_OUTPUTS_ALLOWED 0
#endif
#ifndef VEHICLE_LEGACY_UART
#define VEHICLE_LEGACY_UART 1
#endif
void VehicleApp_Init(void);
void VehicleApp_Tick1ms(void);
void VehicleApp_Receive(uint8_t byte,uint32_t arrived);
void VehicleApp_RxLost(void);
void VehicleApp_LogPoll(void);
typedef struct {
    uint32_t count, age_ms, faults;
    float received_mps;
    uint16_t received_pwm;
    uint8_t seen, fresh, range_ok;
    uint8_t gear, gear_fresh, display_enabled;
    uint8_t gamepad_connected;
} VehicleRxView;
void VehicleApp_GetRxView(VehicleRxView *view);
typedef struct {int32_t delta; uint32_t sampled_at; uint8_t valid;} VehicleEncoderView;
void VehicleApp_GetEncoderView(VehicleEncoderView *view);
/* Only a future reviewed local safety authority may call with true. */
void VehicleApp_SetPermit(int enabled);
#endif
