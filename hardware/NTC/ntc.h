#ifndef __NTC_H
#define __NTC_H
#include "main.h"

float ADC2Resistance(uint32_t adc_value);
float resistance2Temperature(float R1);

#endif