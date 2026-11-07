#include "rt1064_reference.h"
#include <math.h>
#include <string.h>
float RT1064_MmPerCount(void)
{return 3.14159265358979323846f*RT1064_WHEEL_M*1000/(RT1064_ENCODER_COUNTS*RT1064_RATIO);}
float RT1064_CountsToMps(int32_t delta)
{return delta*RT1064_MmPerCount()/1000/RT1064_PERIOD_S;}
float RT1064_TargetCounts(float speed)
{
    if(!isfinite(speed))return 0;
    if(speed>4)speed=4;
    if(speed< -4)speed=-4;
    return speed*RT1064_PERIOD_S*1000/RT1064_MmPerCount();
}
RT1064MotorReference RT1064_MotorCommand(int32_t output)
{
    /* Clamp before absolute value to avoid INT_MIN negation; matches legacy in-range. */
    if(output>1950)output=1950;
    if(output< -1950)output=-1950;
    uint32_t magnitude=(uint32_t)(output<0?-output:output);
    RT1064MotorReference r={(uint16_t)(magnitude*10),(uint16_t)(magnitude/2),(uint8_t)(output>0)};
    return r; /* zero duty: legacy leaves direction unchanged; caller must ignore direction */
}
uint16_t RT1064_ServoCorrect(int32_t command,uint16_t middle,uint16_t left,uint16_t right)
{
    if(left>middle || middle>right || left<500 || right>2500)return 1500;
    int64_t pwm=3000LL-command-((int)middle-1500);
    if(pwm<3000-right)pwm=3000-right;
    if(pwm>3000-left)pwm=3000-left;
    return (uint16_t)pwm;
}
uint16_t RT1064_ServoAngle(float angle)
{
    if(!isfinite(angle))return 1500;
    if(angle>38)angle=38;
    if(angle< -38)angle=-38;
    angle=-angle;
    uint16_t command=(uint16_t)(1500+angle/38*(angle>=0?500:360));
    return (uint16_t)(3000-command);
}
void RT1064_InitBootPID(RT1064_PID *p)
{
    memset(p,0,sizeof(*p));
    RT1064_PID_Init(p,RT1064_Position_PID,1950,-1950,10000,4000,0,0,0);
}
