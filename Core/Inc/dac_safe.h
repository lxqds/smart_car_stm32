#ifndef DAC_SAFE_H
#define DAC_SAFE_H
#include <stdint.h>
#include <math.h>

/* Invalid/non-finite inputs are fail-low; finite positive overrange saturates. */
static inline uint16_t DAC_CodeFromVolts(float volts)
{
    if (!isfinite(volts) || volts <= 0.0f) return 0U;
    if (volts >= 3.3f) return 4095U;
    return (uint16_t)((volts / 3.3f) * 4095.0f + 0.5f);
}
#endif
