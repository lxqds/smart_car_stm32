/*********************************************************************************************************************
* RT1064DVL6A Opensourec Library RT1064DVL6A ????? SDK ????
* Copyright (c) 2022 SEEKFREE ???
*
* ? RT1064DVL6A ???
*
* RT1064DVL6A ? ?
* ?? GPLGNU General Public License GNU?ù?
*  GPL ?3? GPL3.0??????·?/?
*
* ????ã?????
* û??????
* ??? GPL
*
* ??????? GPL ?
* û?<https://www.gnu.org/licenses/>
*
* ?
* ?? GPL3.0 ??? ???
* ?? libraries/doc ?µ GPL3_permission_statement.txt ?
* ? libraries ? ?µ LICENSE ?
* ???ò ???????
*
* ?          main
* ?          ?????
* ??          ? libraries/doc ? version ? ??
*           IAR 8.32.4 or MDK 5.33
* ??          RT1064DVL6A
*           https://seekfree.taobao.com/
*
* ??¼
*                               ?
* 2022-09-21        SeekFree            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "zf_driver_delay.h"
#include "zf_driver_encoder.h"
// µ??????²
// ? ?????
// ? project->clean  ?·

// ???ÿ?
#define UART_INDEX              (DEBUG_UART_INDEX   )                           // ? UART_1
#define UART_BAUDRATE           (DEBUG_UART_BAUDRATE)                           // ? 115200
#define UART_TX_PIN             (DEBUG_UART_TX_PIN  )                           // ? UART1_TX_A9
#define UART_RX_PIN             (DEBUG_UART_RX_PIN  )                           // ? UART1_RX_A10

#define UART_PRIORITY           (LPUART1_IRQn)                                  // ?????  MIMXRT1064.h ???? IRQn_Type ö

uint8 uart_get_data[64];                                                        // ??
uint8 fifo_get_data[64];                                                        // fifo 

uint8 get_data = 0;                                                             // ?
uint32 fifo_data_count = 0;                                                     // fifo ?

fifo_struct uart_data_fifo;
extern u8 PID_init_flag;
extern struct Keys_Struct Keys[4];//
extern Encoder_Init Encoder;
extern uint16_t Edge_HBP;
extern uint16_t Edge_Tran;
#include "servo.h"

extern PID speed_pid;

extern uint8_t Edge_ErrData[USB_FRAME_LENMAX];
extern uint8_t Edge_ErrFlage;
u8 time_hh, time_mm, time_ss;
u16 time_ms;
int main(void)
{
	clock_init(SYSTEM_CLOCK_600M);  // ?
	debug_init();                   // ????

	// ??û ?


	gpio_init(B9, GPO, 1, GPO_PUSH_PULL);


	{//init
		oled_init();
		LED_Init();
		LED_On();
		oled_set_dir(0);
		oled_set_font(0);
		char text[30];

		imu963ra_init();
		encoder_quad_init(QTIMER1_ENCODER1, QTIMER1_ENCODER1_CH1_C0, QTIMER1_ENCODER1_CH2_C1);
		pwm_init(PWM1_MODULE3_CHA_D0, 5000, 0);
		pwm_init(PWM4_MODULE2_CHA_C30, 50, 0);

		fifo_init(&uart_data_fifo, FIFO_DATA_8BIT, uart_get_data, 64);
		uart_init(UART_1, 115200, UART1_TX_B12, UART1_RX_B13);
		uart_init(UART_8, 115200, UART8_TX_D16, UART8_RX_D17);
		SERVO_Init();
		
		GPIO_Initialize();
		USB_Inspector_Init();
		MOTOR_Init();
		ENCODER_Init();
		ICAR_Init();
		USB_Edgeboard_Init();
		// PID?                            
		PID_Init(&speed_pid, Position_PID, 1950, -1950, 10000, 4000, 0, 0, 0);
		flash_init();//

		GPIO_BuzzerEnable(BuzzerSysStart);
		FLASH_LoadAllConfig();

		TIM2_Init();
	}
	while (1) {
		{//??
			GPIO_Handle();
			FLASH_Handle();
			ICAR_Handle();
			// USB_Edgeboard_Handle();
			IMU_Handle();
		}
		//SERVO_SetPwmValueCorrect(1000);
		{
			char text5[30];
			sprintf(text5, "yaw:%.3f,ya:%.3f ", ImuStructure.YAW, ImuStructure.GyroZ);
			oled_show_string(8, 0, text5);
			//sprintf(text5,"E1:%.3d  E2:%.3d",encoder_get_count(QTIMER1_ENCODER1),encoder_get_count(QTIMER1_ENCODER2));
			//oled_show_string(8,5,text5);
			sprintf(text5, "V:%.3f  S:%.3f ", Encoder.Speed[0], Encoder.Distance[0]);
			oled_show_string(8, 6, text5);
			//sprintf(text5,"V2:%.3f ",icarStr.SpeedFeedback);
			//oled_show_string(8,7,text5);
			char text2[30];
			sprintf(text2, "so:%.1d setv:%.2f", icarStr.ServoPwmSet, icarStr.SpeedSet);
			oled_show_string(8, 4, text2);
		}
		{//
			if (Keys[0].Key_Single_Flag == 1) {
				Keys[0].Key_Single_Flag = 0;

				LED_On();
				char text[30];
				sprintf(text, "KEY1Down");
				oled_show_string(0, 1, text);
			}
			if (Keys[1].Key_Single_Flag == 1) {
				Keys[1].Key_Single_Flag = 0;
				GPIO_BuzzerEnable(BuzzerFinish);
				USB_Edgeboard_TransmitKey(120);
				LED_On();
				char text[30];
				sprintf(text, "KEY2Down");
				oled_show_string(0, 1, text);
			}
			if (Keys[2].Key_Single_Flag == 1) {
				GPIO_BuzzerEnable(BuzzerDing);
				Keys[2].Key_Single_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY3Down");
				oled_show_string(0, 1, text);
			}
			if (Keys[0].Key_Double_Flag == 1) {
				Keys[0].Key_Double_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY1DDown");
				oled_show_string(0, 1, text);

			}
			if (Keys[1].Key_Double_Flag == 1) {
				Keys[1].Key_Double_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY2DDown");
				oled_show_string(0, 1, text);

			}
			if (Keys[2].Key_Double_Flag == 1) {
				Keys[2].Key_Double_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY3DDown");
				oled_show_string(0, 1, text);
			}
			if (Keys[0].Key_Long_Flag == 1) {
				Keys[0].Key_Long_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY1LDDown");
				oled_show_string(0, 1, text);

			}
			if (Keys[1].Key_Long_Flag == 1) {
				Keys[1].Key_Long_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY2LDDown");
				oled_show_string(0, 1, text);

			}
			if (Keys[2].Key_Long_Flag == 1) {
				Keys[2].Key_Long_Flag = 0;
				LED_On();
				char text[30];
				sprintf(text, "KEY3LDDown");
				oled_show_string(0, 1, text);
			}
		}

		//?
		{
			if (PID_init_flag == 1) {
				PID_init_flag = 0;

				// imu963ra_init();
				time_ms = 0;time_ss = 0;time_mm = 0;time_hh = 0;
			}

		}

		{//?
			char text[30];
			if (time_ms >= 1000) { time_ms = 0;time_ss++; }
			if (time_ss >= 60) { time_ss = 0;time_mm++; }
			if (time_mm >= 60) { time_mm = 0;time_hh++; }
			sprintf(text, "Time:%d:%d:%d:%d", time_hh, time_mm, time_ss, time_ms);
			oled_show_string(16, 2, text);
		}
		//debug
		{
			if (Edge_ErrFlage) {
				uart_write_buffer(UART_8, Edge_ErrData, USB_FRAME_LENMAX);
				Edge_ErrFlage = false;
			}

		}
	}
}




