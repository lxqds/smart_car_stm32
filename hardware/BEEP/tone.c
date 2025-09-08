#include "tone.h"
#include <stdbool.h>
#include "tim.h"
#define TONE_FREQ_MAX 500000U


static bool Tone_IsContinuousModeEnable = false;

static uint32_t Tone_ToggleCounter = 0;


/**
  * @brief  tone中断入口，被定时中断调用
  * @param  无
  * @retval 无
  */
void Tone_TimerHandler()
{
    if(!Tone_IsContinuousModeEnable)
    {
        if(Tone_ToggleCounter == 0)
        {
            noTone();
            return;
        }

        Tone_ToggleCounter--;
    }
//    togglePin(Tone_Pin);
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_11);
}


/**
  * @brief  在Pin上生成指定频率 (50%占空比的方波)
  * @param  pin: 产生音调的 Pin
  * @param  freq: 频率(Hz)
  * @param  duration: 音调的持续时间 (以毫秒为单位)
  * @retval 无
  */
void tone(uint32_t freq, uint32_t duration)
  {
		//设置无声
      noTone();
  
      if(duration == 0 || freq == 0 || freq > TONE_FREQ_MAX)
      {
          return;
      }
  
      Tone_IsContinuousModeEnable = (duration == TONE_DURATION_INFINITE) ? true : false;
  
      if(!Tone_IsContinuousModeEnable)
      {
          Tone_ToggleCounter = freq * duration / 1000 * 2;
  
          if(Tone_ToggleCounter == 0)
          {
              return;
          }
  
          Tone_ToggleCounter--;
      }
			//时间是1us
//      Timer_SetInterruptTimeUpdate(Tone_Timer, TONE_FREQ_MAX / freq);
//      Timer_SetEnable(Tone_Timer, true);
			__HAL_TIM_SET_AUTORELOAD(&htim13, TONE_FREQ_MAX/ freq);
			HAL_TIM_Base_Start_IT(&htim13);
  }

/**
  * @brief  关闭声音
  * @param  Pin: 产生音调的引脚编号
  * @retval 无
  */
void noTone()
  {
		//1.关闭定时器
//      if(Tone_Timer)
//      {
//          Timer_SetEnable(Tone_Timer, false);
//      }
		
    //2.写低电平
//      digitalWrite(pin, LOW);
//      Tone_IsContinuousModeEnable = false;
//      Tone_Pin = NOT_A_PIN;
//      Tone_ToggleCounter = 0;
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET);
		HAL_TIM_Base_Stop_IT(&htim13);
  }