/* RT1064 0a878262 portable arithmetic reference; original/license in reference/.
 * Not selected by VehicleCore. Retains legacy reset/saturation semantics. */
#ifndef RT1064___PID_H
#define RT1064___PID_H	


#include <stdint.h>


#define VV_DEADLINE 		    2						
#define PID_VKP					2.8f					
#define PID_VKI					0.5f					
#define PID_VKD					0.55f					

#define I_Band1						1000		   
#define I_Band2						 20








































enum
{
	RT1064_Now,
	RT1064_Last,
	RT1064_LLast,
	RT1064_Position_PID,
	RT1064_Delta_PID,
};

typedef struct RT1064_PID
{
	float Kp;
	float Ki;
	float Kd;

	float err[3];
	float err_limit;

	float Pout;
	float Iout;
	float Dout;
	float Out;



	uint8_t PID_Mode;
	float Integral_limit;
	float Max_Out;
	float Min_Out;

} RT1064_PID;


void RT1064_PID_Init(
	RT1064_PID* pid,
	uint8_t PID_Mode,
	float Max_Out,
	float Min_Out,
	float err_limit,
	float Integral_limit,
	float Kp,
	float Ki,
	float Kd);
float RT1064_Pid_Calculate(RT1064_PID* pid, float Target, float Current);
void RT1064_Pid_Reset(RT1064_PID* pid);
void RT1064_Pid_Reset_I(RT1064_PID* pid);

void RT1064_PID_Set(
	RT1064_PID* pid,
	float err_limit,
	float Integral_limit,
	float Kp,
	float Ki,
	float Kd);





#endif


















