#include "rt1064_reference.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void)
{
    assert(fabsf(RT1064_MmPerCount()-0.1454441f)<0.00001f);
    assert(fabsf(RT1064_CountsToMps(100)-1.454441f)<0.0001f);
    assert(fabsf(RT1064_TargetCounts(1)-68.75494f)<0.001f);
    assert(RT1064_TargetCounts(5)==RT1064_TargetCounts(4));
    RT1064MotorReference m=RT1064_MotorCommand(1950);
    assert(m.duty_20000==19500 && m.duty_permille==975 && m.direction_high);
    m=RT1064_MotorCommand(INT32_MIN);assert(m.duty_permille==975 && !m.direction_high);
    assert(RT1064_ServoCorrect(1500,1500,1140,2000)==1500);
    assert(RT1064_ServoCorrect(1140,1500,1140,2000)==1860);
    assert(RT1064_ServoCorrect(2000,1500,1140,2000)==1000);
    assert(RT1064_ServoAngle(38)==1860 && RT1064_ServoAngle(-38)==1000);
    assert(RT1064_ServoAngle(19)==1680 && RT1064_ServoAngle(-19)==1250);
    RT1064_PID p;RT1064_InitBootPID(&p);
    assert(RT1064_Pid_Calculate(&p,100,0)==0); /* real boot gains are zero */
    RT1064_PID_Set(&p,10000,4000,8,.8f,0);
    assert(fabsf(RT1064_Pid_Calculate(&p,10,0)-88)<0.001f);
    assert(RT1064_Pid_Calculate(&p,10000,0)==1950);
    RT1064_Pid_Reset(&p);assert(p.Out==0 && p.Iout==0 && p.err[RT1064_Last]!=0);
    puts("PASS RT1064 reference units, 4m/s clamp, 97.5% PWM, both servo paths, actual boot PID and legacy reset behavior");
}
