/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "comm_uart7.h"
#include "dac_safe.h"
#include "usb_log.h"
/* Keep disabled until separately validated with the user. */
#define CAR_ACTUATORS_ENABLED 0
#define POWER_OUTPUT_ENABLED 0
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_o.h"
#include "stdio.h"
#include "string.h"
#include "tone.h"
#include "AT24C02.h"
#include "ntc.h"

uint8_t key;

float OUT_ADC;
uint16_t ADC_value[5];
uint8_t key;
float U_set =30.00;
float I_set =1.00;
float last_U_set =30.00;
float last_I_set =1.00;
uint8_t mode_set;
//电压设置参数
uint32_t U_set_int = 15;
float U_set_float = 0.00;
uint16_t U_set_count;
uint8_t U_set_mode;
float U_OUT;
uint8_t U_OUT_int;
uint8_t U_OUT_float;
float U_IN;
//电流设置参数
uint32_t I_set_int = 1;
float I_set_float = 0.50;
uint16_t I_set_count;
uint8_t I_set_mode;
float I_OUT;
uint8_t I_OUT_int;
uint8_t I_OUT_float;
//DAC 设置
float DAC_set=1.50f;
float DAC_set_I=0.00f;
//LM5116使能
uint8_t LM5116_EN;
//恒压恒流判断
float I_CCS;
uint8_t mode_CC;
//风扇散热
uint8_t mode_fan;
//过流保护
float I_OCP;
//设置最大电流
uint16_t I_MAX=10;

//调试
extern uint16_t time_1s;
extern uint16_t down_count[5];
extern uint8_t release[5];
//NTC
//通过ADC值计算NTC电阻值
//adc_value 输入ADC结果，范围 0 - 4095
//return 返回NTC电阻值，浮点数类型，单位Ω�
float NTC_R;
float tempture;
uint8_t tempture_u8;

//PID参数
float U_error,last_U_error;
float I_error,last_I_error;

uint16_t ADC2_BUFF[1024];
uint16_t ADC3_BUFF[1024];

uint8_t mode;
void power_grocess();
void power_grocess2();
uint8_t adc_dma_finish;


//car
uint16_t servo_duty;
uint16_t motor_duty;

uint16_t motor_count;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void LCD_ServiceHook(void)
{
    static uint32_t last_service;
    uint32_t now = HAL_GetTick();
    if (now != last_service) {
        Comm_UART7_Poll();
        last_service = HAL_GetTick();
    }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI3_Init();
  MX_TIM14_Init();
  MX_TIM13_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_DAC1_Init();
  MX_TIM1_Init();
#if CAR_ACTUATORS_ENABLED
  MX_TIM2_Init();
  MX_TIM5_Init();
#endif
  /* USER CODE BEGIN 2 */
  Comm_UART7_Init();

	HAL_TIM_Base_Start_IT(&htim13);
	HAL_TIM_Base_Start_IT(&htim14);
	
	LCD_Init();
	LCD_Display_Dir(3);		 		
	LCD_Clear(BLUE);
	LCD_LED(1);
	LCD_DisplayOn();
  USB_Log_Event("APP", "LCD_INIT_DONE MAIN PWM_OFF");
	
	AT24C02_I2C_Init();
	
	 HAL_ADCEx_Calibration_Start(&hadc1,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED);
	 HAL_ADC_Start_DMA(&hadc1,(uint32_t*)ADC_value,5);

	 
	 
	 HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_1);
	 HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_2);
	 //记忆电压读取
    /* Commit persisted settings only after a complete, valid read. */
    uint8_t saved[10];
    if (AT24C_ReadBytes(0, saved, sizeof(saved)) == HAL_OK) {
        float saved_u, saved_i;
        memcpy(&saved_u, &saved[1], sizeof(saved_u));
        memcpy(&saved_i, &saved[6], sizeof(saved_i));
        if (isfinite(saved_u) && isfinite(saved_i) &&
            saved_u >= 0.0f && saved_u < 1.0f &&
            saved_i >= 0.0f && saved_i < 1.0f &&
            saved[0] + saved_u <= 30.0f && saved[5] + saved_i <= I_MAX) {
            U_set_int = saved[0]; U_set_float = saved_u;
            I_set_int = saved[5]; I_set_float = saved_i;
            USB_Log_Event("EEPROM_BOOT", "settings loaded");
        } else USB_Log_Event("EEPROM_BOOT", "invalid data; defaults retained");
    } else USB_Log_Event("EEPROM_BOOT", "read failed; defaults retained");

	 U_set = U_set_int+U_set_float;
	 I_set = I_set_int+I_set_float;
	 U_set_count = U_set_int;
	 __HAL_TIM_SET_COUNTER(&htim3,U_set_count);
	 
	 HAL_DAC_Start(&hdac1,DAC_CHANNEL_1);//开启DAC
	 DAC_set = 1.5f;
	 DAC_set = U_set/(40.00f/4.300f+1);
	 HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
	 
	 DAC_set_I =1.5f;
	 DAC_set_I = I_set*0.005f*50.f+0.6175f;
	 HAL_DAC_Start(&hdac1,DAC_CHANNEL_2);//开启DAC
	 HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set_I));
	 
	 //Car
#if CAR_ACTUATORS_ENABLED
	 HAL_TIM_PWM_Start(&htim5,TIM_CHANNEL_3);//舵机
	 HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);//电机
#endif
	 HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_1);//编码器
	 HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_2);//编码器
	 
	//LCD_Clear(Black);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t ui_tick = HAL_GetTick();
  USB_Log_Event("APP", "MAIN_LOOP_READY PWM_OFF");
  while (1)
  {
    Comm_UART7_Poll();
    if ((uint32_t)(HAL_GetTick() - ui_tick) < 200U) continue;
    ui_tick = HAL_GetTick();
		if(CAR_ACTUATORS_ENABLED && key==31)
		{
			key=0;
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
		}
		
		motor_count = __HAL_TIM_GET_COUNTER(&htim1);
		
		char text[30];
		sprintf(text,"m_count=%d",motor_count);
		LCD_ShowString(160,144,200,24,24,(uint8_t*)text);
		
#if CAR_ACTUATORS_ENABLED
		motor_duty= U_set_float*20000;
		if(motor_duty>10000)
		{
			motor_duty = 10000;
		}
		__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,motor_duty);
		servo_duty = (10 * (I_set_float/1.0f)+2.5f)/100.0f*20000;
		__HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_3,servo_duty);
#endif
		
		
        power_grocess2();
        USB_Log_Event("UI_PROGRESS", "cycle complete PWM_OFF");
        static uint32_t seen_errors;
        static HAL_StatusTypeDef seen_status = HAL_OK;
        if (seen_errors != AT24C_ErrorCount() || seen_status != AT24C_LastStatus()) {
            char state[48];
            seen_errors = AT24C_ErrorCount(); seen_status = AT24C_LastStatus();
            snprintf(state, sizeof(state), "status=%u failures=%lu", (unsigned)seen_status,
                     (unsigned long)seen_errors);
            USB_Log_Event("EEPROM_STATE", state);
        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20; /* USB 48 MHz; SPI3 also uses PLL1Q. */
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void power_grocess2()
{
		uint8_t temp = 0;
		char text[30];
		if(key!=0)
		{
			sprintf(text,"key=%d",key);
			LCD_ShowString(160,0,200,24,24,(uint8_t*)text);
		}

		sprintf(text,"TEST%d",temp);
		LCD_ShowString(0,0,200,24,24,(uint8_t*)text);
		U_IN = ((ADC_value[0]*3300)>>16)*(200.0f/10.0f+1)/1000.0f;
		sprintf(text,"IN_U :%.2fV",U_IN);
		LCD_ShowString(160,24,200,24,24,(uint8_t*)text);
	
		//小屏幕版本电阻，U_in 200k/20k I_in (3+1)/1
		//大屏幕版本电阻，U_in 40k/4.3k I_in (4.3+4.3)/4.3
		U_OUT = ((ADC_value[1]*3300)>>16)*(200.00f/20.0f+1)/1000.f;
		sprintf(text,"OUT_U:%.2fV",U_OUT);
		LCD_ShowString(160,48,200,24,24,(uint8_t*)text);
		I_OUT = ((ADC_value[2]*3.300f/65535*((3.0f+1.0f)/1.00f+1.0f))-0.6175f)/50.0f/0.005f;
		sprintf(text,"OUT_I:%.3fA ",I_OUT);
		LCD_ShowString(160,72,200,24,24,(uint8_t*)text);
		I_CCS = ADC_value[3]*3.300f/65535;
		sprintf(text,"CCS  :%.2fV",I_CCS);
		LCD_ShowString(160,96,200,24,24,(uint8_t*)text);
		
		NTC_R=ADC2Resistance(ADC_value[4]);
		tempture = resistance2Temperature(NTC_R);
		sprintf(text,"NTC  :%.2f C",tempture);
		LCD_ShowString(160,120,200,24,24,(uint8_t*)text);

		
		//判读恒压模式还是恒流模式
		if(I_CCS>1.5f)
		{
			mode_CC=0;
		}else if(I_CCS<1.5f)
		{
			mode_CC=1;
		}
		
		//温度过高开启风扇
		if(tempture>50&&mode_fan==0)
		{
			mode_fan=1;
		}else if(tempture<48&&mode_fan==1)
		{
			mode_fan=0;
		}
		if(mode_fan==1)
		{
			HAL_GPIO_WritePin(GPIOE,GPIO_PIN_9,GPIO_PIN_SET);
		}else
		{
			HAL_GPIO_WritePin(GPIOE,GPIO_PIN_9,GPIO_PIN_RESET);
		}
		//过流保护	
		if(I_OUT>I_set+0.5f|U_OUT<0.001)
		{
//			tone(2000,20);
		  LM5116_EN=0;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,GPIO_PIN_RESET);//LM5116使能引脚
		}
		
		if(POWER_OUTPUT_ENABLED && key ==41)
		{
			key = 0;
			if(LM5116_EN==0)
			{
				tone(1500,20);
				LM5116_EN=1;
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,GPIO_PIN_SET);//LM5116使能引脚
				//软启动
				DAC_set = U_set/9/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/9/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/8/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/7/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/6/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/5/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/4/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(50);
				DAC_set = U_set/3/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(100);
				DAC_set = U_set/2/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
				HAL_Delay(100);
				DAC_set = U_set/10.3023255;
				HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
			}else if (LM5116_EN==1)
			{
				tone(1000,20);
				LM5116_EN=0;
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,GPIO_PIN_RESET);//LM5116使能引脚
			}
		}
		//电压电流切换
		if(key==11)
		{
			tone(700,20);
			key = 0;
			mode_set --;
			mode_set%=2;
			if(mode_set==0)//切换到电压设置时,先调整整数
			{
				U_set_mode =0;
				U_set_count = U_set_int;
				__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
			}else if(mode_set==1)//切换到电流设置时,先调整小数
			{
				I_set_mode =1;
				I_set_count = I_set_float;
				__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
			}
		}else if(key==21)
		{
			tone(500,20);
			key = 0;
			mode_set ++;
			mode_set%=2;
			if(mode_set==0)//切换到电压设置时,先调整整数
			{
				U_set_mode =0;
				U_set_count = U_set_int;
				__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
			}else if(mode_set==1)//切换到电流设置时,先调整小数
			{
				I_set_mode =1;
				I_set_count = I_set_float*100;
				__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
			}
		}
		sprintf(text,"M%d ",mode_set);
		LCD_ShowString(112,0,200,24,24,(uint8_t*)text);
		switch(mode_set)//根据modeset调整设置电压或电流
		{
			case 0:
			{
				//切换模式
				if(key==51)
				{
					tone(900,20);
					key = 0;
					if(U_set_mode==0)//0是电压整数，1是电压小数
					{
						U_set_mode =1;
						U_set_count = U_set_float*100;
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}else if(U_set_mode==1)
					{
						U_set_mode =0;
						U_set_count = U_set_int;
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}
				}
				
				if(U_set_mode==0)//电压整数调整
				{
					U_set_count=__HAL_TIM_GET_COUNTER(&htim3);
					if(U_set_count>1000)
					{
						U_set_count = 0;
						U_set_float = 0;
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}else if(U_set_count>=30)
					{
						U_set_count = 30;
						U_set_float = 0;
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}
					U_set_int = U_set_count;
					//写入电压整数
					AT24C_WriteOneByte(0,U_set_int);
					HAL_Delay(5);
				}else if(U_set_mode==1)//电压小数调整
				{
					static uint16_t last_U_set_count;
					U_set_count=__HAL_TIM_GET_COUNTER(&htim3);
					if(U_set_count!=last_U_set_count)
					{
						last_U_set_count = U_set_count;
					}
					
					if(U_set_count>200)
					{
						U_set_count = 99;
						U_set_int--;
						if(U_set_int>100)//防止整数部分越界
						{
							U_set_count = 0;//整数部分为0时，小数部分不再减小
							U_set_int=0;
						}
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}else if(U_set_count>99)
					{
						U_set_count = 0;
						U_set_int++;
						if(U_set_int>=30)
						{
							U_set_int=30;
						}
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}
					if(U_set_int==30)//超过30时，不能再增加小数
					{
						U_set_count = 0;
						__HAL_TIM_SET_COUNTER(&htim3,U_set_count);
					}
					U_set_float = U_set_count/100.f;
					//电压小数存储
					uint32_t temp_32;
					uint8_t temp_8[4];
					temp_32 = MCM_floatToIntBit(U_set_float);
					temp_8[0]=temp_32;
					temp_8[1]=temp_32>>8;
					temp_8[2]=temp_32>>16;
					temp_8[3]=temp_32>>24;
					AT24C_WriteOneByte(1,temp_8[0]);
					HAL_Delay(5);
					AT24C_WriteOneByte(2,temp_8[1]);
					HAL_Delay(5);
					AT24C_WriteOneByte(3,temp_8[2]);
					HAL_Delay(5);
					AT24C_WriteOneByte(4,temp_8[3]);
				}
			}break;
			case 1:
			{
				//电流模式调整
				if(key==51)
				{
					tone(900,20);
					key = 0;
					if(I_set_mode==0)//0是电流整数，1是电流小数
					{
						I_set_mode =1;
						I_set_count = I_set_float*100;
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}else if(I_set_mode==1)
					{
						I_set_mode =0;
						I_set_count = I_set_int;
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}
				}
				
				if(I_set_mode==0)//电流整数调整
				{
					I_set_count=__HAL_TIM_GET_COUNTER(&htim3);
					if(I_set_count>1000)
					{
						I_set_count = 0;
						I_set_float = 0;
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}else if(I_set_count>=I_MAX)
					{
						I_set_count = I_MAX;
						I_set_float = 0;
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}
					I_set_int = I_set_count;
					AT24C_WriteOneByte(5,I_set_int);
					HAL_Delay(5);
				}else if(I_set_mode==1)//电流小数调整
				{
					I_set_count=__HAL_TIM_GET_COUNTER(&htim3);
					if(I_set_count>200)
					{
						I_set_count = 99;
						I_set_int--;
						if(I_set_int>100)//防止整数部分越界
						{
							I_set_count = 0;//整数部分为0时，小数部分不再减小
							I_set_int=0;
						}
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}else if(I_set_count>99)
					{
						I_set_count = 0;
						I_set_int++;
						if(I_set_int>=I_MAX)
						{
							I_set_int=I_MAX;
						}
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}
					if(I_set_int==I_MAX)//超过3时，不能再增加小数
					{
						I_set_count = 0;
						__HAL_TIM_SET_COUNTER(&htim3,I_set_count);
					}
					I_set_float = I_set_count/100.f;
					//电流小数存储
					uint32_t temp_32;
					uint8_t temp_8[4];
					temp_32 = MCM_floatToIntBit(I_set_float);
					temp_8[0]=temp_32;
					temp_8[1]=temp_32>>8;
					temp_8[2]=temp_32>>16;
					temp_8[3]=temp_32>>24;
					AT24C_WriteOneByte(6,temp_8[0]);
					HAL_Delay(5);
					AT24C_WriteOneByte(7,temp_8[1]);
					HAL_Delay(5);
					AT24C_WriteOneByte(8,temp_8[2]);
					HAL_Delay(5);
					AT24C_WriteOneByte(9,temp_8[3]);
				}
			}break;
		}
		
		U_set= U_set_int+U_set_float;
		I_set= I_set_int+I_set_float;
    sprintf(text,"U_set:%.2f %d",U_set,U_set_mode);
		LCD_ShowString(0,48,200,24,24,(uint8_t*)text);
		sprintf(text,"I_set:%.2f %d",I_set,I_set_mode);
		LCD_ShowString(0,72,200,24,24,(uint8_t*)text);
		//记录保存上一次的电压、电流
		if(U_set!=last_U_set)
		{
			last_U_set = U_set;
			DAC_set = U_set/(40.00f/4.300f+1);
			
		}
		
		if(I_set!=last_I_set)
		{
			last_I_set = I_set;
			DAC_set_I = I_set*0.005f*50.f+0.6175f;
			
		}
		//分为恒压模式调节和恒流模式调节
		//PID

//		if(mode_CC==0)//恒压模式
//		{
//			U_error = U_OUT-U_set;//计算电压误差
//			if(U_error>0.005)//启动电压误差PID调节
//			{
//				set_pid_target(&volitage_pid,0.005);
//				volitage_pid.output = PID_realize(&volitage_pid,U_error);
//				//增量限幅
//				_Fsat(volitage_pid.output,0.05,0.0009);

//				DAC_set=DAC_set+volitage_pid.output;
//			}
//			last_U_error=U_error;
//		}else if(mode_CC==1)//恒流模式
//		{
//			set_pid_target(&current_pid,I_set);
//			PID_realize(&current_pid,I_OUT);
//		}
		//步进调节法
			if(mode_CC==0)//恒压模式
			{
				U_error = U_OUT-U_set;//计算电压误差
	//			//保留三位小数算法
	//			uint32_t temp;
	//			temp=(uint32_t)(U_error*1000);
	//			U_error=temp/1000.f;

				if(U_error!=last_U_error)
				{
					if(U_error>0.001)//启动电压误差PID调节
					{
						DAC_set=DAC_set-0.0005;
					}else if(U_error<-0.001)
					{
						DAC_set=DAC_set+0.0005;
					}
					last_U_error=U_error;
					
					}
			}else if(mode_CC==1)//恒流模式
			{
				I_error = I_OUT-I_set;//计算电流误差
				if(I_error!=last_I_error)
				{
					if(I_error>0.001)//启动电流误差PID调节
					{
						DAC_set_I=DAC_set_I-0.0005;
					}else if(U_error<-0.001)
					{
						DAC_set_I=DAC_set_I+0.0005;
					}
					last_I_error=I_error;
				}
			}

		//输出电压限幅
		if (!isfinite(DAC_set) || DAC_set < 0.0f) DAC_set = 0.0f;
		if(DAC_set>3.30000f)
		{
			DAC_set=3.30000f;
		}
		HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_1,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set));
		//输出电流限幅
		if (!isfinite(DAC_set_I) || DAC_set_I < 0.0f) DAC_set_I = 0.0f;
		if(DAC_set_I>3.30000f)
		{
			DAC_set_I=3.30000f;
		}
		HAL_DAC_SetValue(&hdac1,DAC_CHANNEL_2,DAC_ALIGN_12B_R,DAC_CodeFromVolts(DAC_set_I));
		uint32_t dac_temp;
		dac_temp = HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_1);
		sprintf(text,"DAC_U:%.2fV",dac_temp*3.3f/4096);
		LCD_ShowString(0,96,200,24,24,(uint8_t*)text);
		
		dac_temp = HAL_DAC_GetValue(&hdac1,DAC_CHANNEL_2);
		sprintf(text,"DAC_I:%.2fV",dac_temp*3.3f/4096);
		LCD_ShowString(0,120,200,24,24,(uint8_t*)text);
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
