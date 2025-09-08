#include "ntc.h"


float ADC2Resistance(uint32_t adc_value) 
{
  return (adc_value / (65535.0f - adc_value)) * 10000.0f;
}

//通过NTC阻值计算温度
//R1 NTC电阻值
//return 返回温度，float类型，单位摄氏度
float resistance2Temperature(float R1) 
{
  float B = 3950.0f;
  float R2 = 10000.0f;
  float T2 = 25.0f;
  return (1.0 / ((1.0 / B) * log(R1 / R2) + (1.0 / (T2 + 273.15))) - 273.15);
}

