#ifndef VEHICLE_DISPLAY_H
#define VEHICLE_DISPLAY_H
#include "vehicle_app.h"
/* Display-only inverse of old main mapping: PWM=1500-angle_command*3.
 * NOT a calibrated physical angle and never used by actuator code. */
void VehicleDisplay_Format(const VehicleRxView *v,char lines[4][41]);
void VehicleDisplay_Poll(void);
#endif
