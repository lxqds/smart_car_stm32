#ifndef VEHICLE_CORE_H
#define VEHICLE_CORE_H
#include <stdint.h>
#include <stddef.h>
/* Portable, units: m/s, mm, microseconds and signed duty permille. */
typedef struct {
    float mm_per_count, speed_limit_mps, kp, ki, kd;
    int encoder_sign, motor_sign, servo_reverse;
    uint16_t servo_min_us, servo_center_us, servo_max_us;
    uint16_t duty_limit_permille, encoder_max_delta;
    uint32_t command_timeout_ms;
    uint8_t calibrated;
} VehicleConfig;
enum { VEHICLE_FAULT_CONFIG=1, VEHICLE_FAULT_ENCODER=2, VEHICLE_FAULT_TIMING=4,
       VEHICLE_FAULT_NUMERIC=8 };
typedef struct {
    VehicleConfig config;
    uint16_t previous_count, servo_us;
    int32_t encoder_delta;
    int64_t total_counts;
    /* Raw diagnostic only: independent of calibration/permit, reset at boot. */
    int64_t encoder_raw_total;
    uint32_t encoder_changes;
    uint8_t encoder_diag_invalid;
    float speed_mps, distance_mm, target_mps, integral, last_measurement;
    int16_t motor_permille;
    uint32_t last_control_ms, faults;
    uint8_t permit, neutral_seen, have_control;
    uint16_t command_servo_us;
} VehicleCore;
int Vehicle_ConfigValid(const VehicleConfig *c);
void Vehicle_Init(VehicleCore *v,const VehicleConfig *c,uint16_t counter);
void Vehicle_SetPermit(VehicleCore *v,int permit);
void Vehicle_Stop(VehicleCore *v);
int Vehicle_Command(VehicleCore *v,float speed_mps,uint16_t servo_us,uint32_t arrived,uint32_t now);
void Vehicle_Step10ms(VehicleCore *v,uint16_t counter,uint32_t now,uint32_t elapsed_ms);
typedef void (*LegacyControlHandler)(void *,float,uint16_t,uint32_t);
typedef struct {
    uint8_t data[30], used;
    uint32_t arrived[30]; /* original RX time paired with each buffered byte */
    uint32_t last_byte_ms, good, bad, unsupported;
    uint32_t display_at;
    uint8_t display_gear, display_seen, display_enabled; /* Y gates motor; separate freshness */
    uint32_t gamepad_at;
    uint8_t gamepad_seen,gamepad_connected;
} LegacyControlParser;
void Legacy_Reset(LegacyControlParser *p);
void Legacy_Feed(LegacyControlParser *p,uint8_t byte,uint32_t arrived,
                 LegacyControlHandler handler,void *context);
#endif
