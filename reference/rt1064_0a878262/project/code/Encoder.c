#include "encoder.h"

Encoder_Init Encoder;
extern float current_speed;
/*
********************************************************************************************************
*                                               示例代码
*                                             EXAMPLE  CODE
*
*                             (c) Copyright 2021; SaiShu.Lcc.; Leo
*                                 版权所属[北京赛曙科技有限公司]
*
*               The code is for internal use only, not for commercial transactions(开源学习,请勿商用).
*               The code ADAPTS the corresponding hardware circuit board(代码使用CarDo智控板),
*               the specific details consult the professional(欢迎联系我们).
*********************************************************************************************************
*/

/**
* @brief        编码器初始化
* @param
* @ref
* @author       Leo
* @note
**/
void ENCODER_Init(void)
{
  encoder_quad_init(QTIMER1_ENCODER1, QTIMER1_ENCODER1_CH1_C0, QTIMER1_ENCODER1_CH2_C1);
}


/**
* @brief        编码器转速采样
* @param
* @ref
* @author       Leo
* @note
**/
void ENCODER_RevSample(void)
{
  motorStr.EncoderValue = -encoder_get_count(QTIMER1_ENCODER1);

  //计算一次计数值对应的距离和10ms内的速度	
  Encoder.Distance_Once[0] = (float)(motorStr.EncoderValue * PI * motorStr.DiameterWheel) / (motorStr.EncoderLine * motorStr.ReductionRatio);
  Encoder.Speed[0] = Encoder.Distance_Once[0] / 0.01f;

  Encoder.Distance[0] = Encoder.Distance[0] + Encoder.Distance_Once[0];
  encoder_clear_count(QTIMER1_ENCODER1);

  //    if(motorStr.EncoderValue > 32767)
  //        motorStr.EncoderValue = motorStr.EncoderValue - 65536;

  //		motorStr.EncoderValue = motorStr.EncoderValue*4.0f;


      //PID负反馈数据输入
      // pidStr.vi_FeedBack = motorStr.EncoderValue;
  current_speed = motorStr.EncoderValue;

  //计算实际速度	---		m/s
  //icarStr.SpeedFeedback = (float)(motorStr.EncoderValue * PI * motorStr.DiameterWheel)/ MOTOR_CONTROL_CYCLE / motorStr.EncoderLine / 4.0f / motorStr.ReductionRatio; //  m/s
  icarStr.SpeedFeedback = (float)(motorStr.EncoderValue * PI * motorStr.DiameterWheel) / MOTOR_CONTROL_CYCLE / motorStr.EncoderLine / motorStr.ReductionRatio; //  m/s


  if (icarStr.SpeedFeedback > 0 && icarStr.SpeedFeedback > icarStr.SpeedMaxRecords)
    icarStr.SpeedMaxRecords = icarStr.SpeedFeedback;

  else if (icarStr.SpeedFeedback < 0 && -icarStr.SpeedFeedback > icarStr.SpeedMaxRecords)
    icarStr.SpeedMaxRecords = -icarStr.SpeedFeedback;
}




