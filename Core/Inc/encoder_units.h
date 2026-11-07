#ifndef ENCODER_UNITS_H
#define ENCODER_UNITS_H
#include <stdint.h>
/* Legacy geometry/model estimate, NOT verified calibration.
 * RT QTMR counts primary rising edges; STM32 TIM1 TI12 counts x4.
 * Assumes same encoder/mechanics and valid quadrature at STM32 inputs.
 * Preserve fractional counts; never integer-divide each sample by four.
 */
#define ENCODER_WHEEL_DIAMETER_MM 64.0f
#define ENCODER_LEGACY_COUNTS_PER_TURN (512.0f * 2.7f)
#define ENCODER_STM32_COUNTS_PER_TURN (ENCODER_LEGACY_COUNTS_PER_TURN * 4.0f)
#define ENCODER_EST_MM_PER_COUNT (3.14159265358979323846f * ENCODER_WHEEL_DIAMETER_MM / ENCODER_STM32_COUNTS_PER_TURN)
static inline float Encoder_EstSpeedMps(int32_t delta, uint32_t elapsed_ms)
{
    return elapsed_ms ? (float)delta * ENCODER_EST_MM_PER_COUNT / (float)elapsed_ms : 0.0f;
}
static inline float Encoder_EstDistanceM(int64_t total)
{
    return (float)total * ENCODER_EST_MM_PER_COUNT / 1000.0f;
}
#endif
