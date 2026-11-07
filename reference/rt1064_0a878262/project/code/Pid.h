#ifndef __PID_H
#define __PID_H	


#include "zf_common_headfile.h"

/******************************************************************************/
#define VV_DEADLINE 		    2						//速度PID，设置死区范围，消抖，静止强硬程度
#define PID_VKP					2.8f					//PID标定值1.6
#define PID_VKI					0.5f					//PID标定值0.5
#define PID_VKD					0.55f					//PID标定值0.02

#define I_Band1						1000		   //积分项上下限限制，待调......
#define I_Band2						 20

// /**
// * @brief    PID相关
// **/
// typedef struct 
// {
// 	float vi_Ref;        	            //速度PID，速度设定值
// 	float vi_FeedBack;                  //速度PID，速度反馈值

//   float vi_PreError;	 	            //速度PID，速度误差,vi_Ref - vi_FeedBack

// 	float vi_PreDerror; 	            //速度PID，前一次，速度误差之差

// 	float v_Kp;	 		 			    //比例系数，Kp = Kp
// 	float v_Ki;		 				    //积分系数，Ki = Kp * ( T / Ti )
// 	float v_Kd;		 				    //微分系数，Kd = KP * Td * T

// 	float v_Kp_Out;	 		 			    
// 	float v_Ki_Out;		 				    
// 	float v_Kd_Out;		 				    


// 	float vl_PreU; 				        //PID输出值


// }PIDStruct;

// extern PIDStruct pidStr;

// void PID_Init(void);
// signed int PID_MoveCalculate(PIDStruct *pp);











enum
{
	Now,
	Last,
	LLast,
	Position_PID,
	Delta_PID,
};

typedef struct PID
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

} PID;


void PID_Init(
	PID* pid,
	uint8_t PID_Mode,
	float Max_Out,
	float Min_Out,
	float err_limit,
	float Integral_limit,
	float Kp,
	float Ki,
	float Kd);
float Pid_Calculate(PID* pid, float Target, float Current);
void Pid_Reset(PID* pid);
void Pid_Reset_I(PID* pid);

void PID_Set(
	PID* pid,
	float err_limit,
	float Integral_limit,
	float Kp,
	float Ki,
	float Kd);





#endif



//typedef struct 
//{
//	float vi_Ref;        	            //速度PID，速度设定值
//	float vi_FeedBack;                  //速度PID，速度反馈值
//    float vi_PreError;	 	            //速度PID，速度误差,vi_Ref - vi_FeedBack
//	float vi_PreDerror; 	            //速度PID，前一次，速度误差之差，d_error-PreDerror;
//	float v_Kp;	 		 			    //比例系数，Kp = Kp
//	float v_Ki;		 				    //积分系数，Ki = Kp * ( T / Ti )
//	float v_Kd;		 				    //微分系数，Kd = KP * Td * T
//	float vl_PreU; 				        //PID输出值
//	
//	
//}PIDStruct;


