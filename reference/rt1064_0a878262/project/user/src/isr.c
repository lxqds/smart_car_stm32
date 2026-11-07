/*********************************************************************************************************************
* RT1064DVL6A Opensourec Library ����RT1064DVL6A ��Դ�⣩��һ�����ڹٷ� SDK �ӿڵĵ�������Դ��
* Copyright (c) 2022 SEEKFREE ��ɿƼ�
*
* ���ļ��� RT1064DVL6A ��Դ���һ����
*
* RT1064DVL6A ��Դ�� ���������
* �����Ը���������������ᷢ���� GPL��GNU General Public License���� GNUͨ�ù�������֤��������
* �� GPL �ĵ�3�棨�� GPL3.0������ѡ��ģ��κκ����İ汾�����·�����/���޸���
*
* ����Դ��ķ�����ϣ�����ܷ������ã�����δ�������κεı�֤
* ����û�������������Ի��ʺ��ض���;�ı�֤
* ����ϸ����μ� GPL
*
* ��Ӧ�����յ�����Դ���ͬʱ�յ�һ�� GPL �ĸ���
* ���û�У������<https://www.gnu.org/licenses/>
*
* ����ע����
* ����Դ��ʹ�� GPL3.0 ��Դ����֤Э�� ������������Ϊ���İ汾
* ��������Ӣ�İ��� libraries/doc �ļ����µ� GPL3_permission_statement.txt �ļ���
* ����֤������ libraries �ļ����� �����ļ����µ� LICENSE �ļ�
* ��ӭ��λʹ�ò����������� ���޸�����ʱ���뱣����ɿƼ��İ�Ȩ����������������
*
* �ļ�����          isr
* ��˾����          �ɶ���ɿƼ����޹�˾
* �汾��Ϣ          �鿴 libraries/doc �ļ����� version �ļ� �汾˵��
* ��������          IAR 8.32.4 or MDK 5.33
* ����ƽ̨          RT1064DVL6A
* ��������          https://seekfree.taobao.com/
*
* �޸ļ�¼
* ����              ����                ��ע
* 2022-09-21        SeekFree            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "zf_common_debug.h"
#include "isr.h"

struct Keys_Struct Keys[4];

extern uint16_t Edge_HBP;
extern uint16_t Edge_Tran;
extern u16 time_ms;

uint8_t Edge_ErrData[USB_FRAME_LENMAX];
uint8_t Edge_ErrFlage = 0;
void CSI_IRQHandler(void)
{
    CSI_DriverIRQHandler();     // ����SDK�Դ����жϺ��� ���������������������õĻص�����
    __DSB();                    // ����ͬ������
}

void PIT_IRQHandler(void)
{
    if (pit_flag_get(PIT_CH0)) {//PT0��ʱ10ms

//		key_scanner();

        {
            Keys[0].Key_State = gpio_get_level(C31);
            Keys[1].Key_State = gpio_get_level(C27);
            Keys[2].Key_State = gpio_get_level(C26);
        }
        for (uint8_t i = 0;i < 3;i++) {//ѭ��3�������ж�
            if (Keys[i].Time_Count_Flag == 1) {//�����ʼ��ʱ��־λ��1���ǾͿ�ʼ��ʱ
                Keys[i].Press_Time_Count++;
            }
            else {
                Keys[i].Press_Time_Count = 0;
            }
            switch (Keys[i].Step) {//����
                case 0:
                {
                    if (Keys[i].Key_State == 0) {//���а�������ʱ�����벽��1
                        Keys[i].Step = 1;
                        Keys[i].Time_Count_Flag = 1;
                    }
                }break;
                case 1:
                {
                    if (Keys[i].Press_Time_Count > 2 && Keys[i].Key_State == 0) {//��ʱ20ms�ж�ʱ�䵽��֮���Ұ���δ�ɿ�
    //					Keys[i].Time_Count_Flag = 0;�Ȳ����㣬������ʱ
                        Keys[i].Step = 2;
                        //					Keys[i].Key_Single_Flag = 1;
                    }
                }break;
                case 2:
                {
                    if (Keys[i].Key_State == 1) {//�ɿ�����
                        if (Keys[i].Press_Time_Count < 50) {//����ɿ�ʱʱ����0.5s֮ǰ�����ж��ǵ�������˫��
                            Keys[i].Step = 3;
                            //						Keys[i].Time_Count_Flag = 0;
                            //						Keys[i].Key_Single_Flag = 1;
                        }
                        if (Keys[i].Press_Time_Count > 100) {
                            Keys[i].Step = 0;
                            Keys[i].Time_Count_Flag = 0;
                            Keys[i].Key_Long_Flag = 1;
                        }
                    }
                }break;
                case 3:
                {
                    if (Keys[i].Key_State == 0 && Keys[i].Press_Time_Count < 50) {//�����ֱ�����ͬʱʱ��С�ڰ���
                        Keys[i].Step = 4;
                        Keys[i].Time_Count_Flag = 0;
                        Keys[i].Key_Double_Flag = 1;
                    }
                    else if (Keys[i].Key_State == 1 && Keys[i].Press_Time_Count == 30) {
                        Keys[i].Step = 4;
                        Keys[i].Time_Count_Flag = 0;
                        Keys[i].Key_Single_Flag = 1;
                    }
                }break;
                case 4:
                {
                    if (Keys[i].Key_State == 1) {
                        Keys[i].Step = 0;
                    }
                }break;
            }
        }

        pit_flag_clear(PIT_CH0);
    }

    if (pit_flag_get(PIT_CH1)) {//PT1  1ms

        static uint16_t i = 0;
        i++;
        if (i == 500) {
            i = 0;
            LED_REV;
        }


        GPIO_Timer();			//GPIO�����߳�
        MOTOR_Timer();
        ICAR_Timer();
        IMU_Timer();
        time_ms++;

        //USBͨ���߳�Keys[2].Key_State = gpio_get_level(C26);
                        //���ܳ��ۺϴ����̼߳�����Keys[1].Key_State = gpio_get_level(C27);

        pit_flag_clear(PIT_CH1);
    }

    if (pit_flag_get(PIT_CH2)) {//PT1  10ms

        USB_Edgeboard_Timr();

        pit_flag_clear(PIT_CH2);
    }

    if (pit_flag_get(PIT_CH3)) {
        pit_flag_clear(PIT_CH3);
    }

    __DSB();
}
extern u8 PID_init_flag;
extern PID speed_pid;
extern u8 echo_data;
void LPUART1_IRQHandler(void)
{

    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART1)) {
        uint8_t dat;
        static uint8_t dat_buf[30];
        static uint8_t dat_buf_index = 0;
        dat = uart_read_byte(UART_1);

        if (dat_buf_index == 0) {
            if (dat == 0x24) {
                memset(dat_buf, 0x00, 30);
                dat_buf[0] = dat;
                dat_buf_index++;
            }
        }
        else if (dat_buf_index == 1) {
            if (dat == 0x42) {
                dat_buf[1] = dat;
                dat_buf[3] = 30;
                dat_buf_index++;
            }
            else if (dat == 0x24) dat_buf_index = 1;
            else dat_buf_index = 0;
        }
        else if (dat_buf_index < dat_buf[3] && dat_buf_index < 30) {
            dat_buf[dat_buf_index] = dat;
            dat_buf_index++;
        }
        else {//接受完数据
            uint8_t check = 0;
            for (uint8_t i = 0; i < dat_buf_index - 1; i++) {
                check += dat_buf[i];
            }
            if (check == dat_buf[dat_buf_index - 1]) {//校验成功
                memset(usbStr.receiveBuffFinished, 0x00, 30);
                memcpy(usbStr.receiveBuffFinished, dat_buf, dat_buf_index);
                //智能车控制指令特殊处理（保障实时性）
                if (USB_ADDR_CONTROL == usbStr.receiveBuffFinished[2]) {
                    Edge_Tran = 1;
                    Edge_HBP = 0;
                    Bint16_Union bint16_Union;
                    Bint32_Union bint32_Union;
                    for (int i = 0;i < 4;i++)
                        bint32_Union.U8_Buff[i] = usbStr.receiveBuffFinished[4 + i];
                    bint16_Union.U8_Buff[0] = usbStr.receiveBuffFinished[8];
                    bint16_Union.U8_Buff[1] = usbStr.receiveBuffFinished[9];
                    SERVO_SetPwmValueCorrect(bint16_Union.U16);
                    icarStr.ServoPwmSet = bint16_Union.U16;         //方向
                    icarStr.SpeedSet = bint32_Union.Float;          //速度	
                }
                else if (USB_ADDR_SETPID == usbStr.receiveBuffFinished[2]) {
                    echo_data = 0XA1;//	pid回响
                    PID_init_flag = 1;
                    float speedPID[3];
                    memcpy(speedPID, &usbStr.receiveBuffFinished[4], 12);
                    PID_Set(&speed_pid, 10000, 4000, speedPID[0], speedPID[1], speedPID[2]);
                    GPIO_BuzzerEnable(BuzzerFinish);
                }
                else if (USB_ADDR_RESETPID_I == usbStr.receiveBuffFinished[2]) {
                    echo_data = 0XA2;//	pid回响
                    PID_init_flag = 1;
                    Pid_Reset_I(&speed_pid);
                    GPIO_BuzzerEnable(BuzzerOk);
                }
                else if (USB_ADDR_BUZZER == usbStr.receiveBuffFinished[2]) {
                    if (usbStr.receiveBuffFinished[4] == 1)          //OK
                        GPIO_BuzzerEnable(BuzzerOk);
                    else if (usbStr.receiveBuffFinished[4] == 2)     //Warnning
                        GPIO_BuzzerEnable(BuzzerWarnning);
                    else if (usbStr.receiveBuffFinished[4] == 3)     //Finish
                        GPIO_BuzzerEnable(BuzzerFinish);
                    else if (usbStr.receiveBuffFinished[4] == 4)     //Ding
                        GPIO_BuzzerEnable(BuzzerDing);
                    else if (usbStr.receiveBuffFinished[4] == 5)     //SystemStart
                        GPIO_BuzzerEnable(BuzzerSysStart);
                }

                // uart_write_string(UART_8, "aaa");
                usbStr.counterDrop = 0;
                if (!usbStr.connected) {//上位机初次连接通信
                    // GPIO_BuzzerEnable(BuzzerOk);
                    usbStr.connected = true;
                }
                // uart_write_buffer(UART_8, usbStr.receiveBuffFinished, 30);
            }
            else {
                uint8_t err_buf[30];
                memset(err_buf, 0x00, 30);
                memcpy(err_buf, dat_buf, dat_buf_index);
                // uart_write_buffer(UART_8, err_buf, dat_buf_index);
            }
            dat_buf_index = 0;
        }
        // 接收中断
#if DEBUG_UART_USE_INTERRUPT                        // 如果开启 debug 串口中断
        debug_interrupr_handler();                      // 调用 debug 串口接收处理函数 数据会被 debug 环形缓冲区读取
#endif                                              // 如果修改了 DEBUG_UART_INDEX 那这段代码需要放到对应的串口中断去

    }
    LPUART_ClearStatusFlags(LPUART1, kLPUART_RxOverrunFlag);    // 不允许删除
}

void LPUART2_IRQHandler(void)
{
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART2)) {
        // �����ж�

    }

    LPUART_ClearStatusFlags(LPUART2, kLPUART_RxOverrunFlag);    // ������ɾ��
}

void LPUART3_IRQHandler(void)
{
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART3)) {
        // �����ж�

    }

    LPUART_ClearStatusFlags(LPUART3, kLPUART_RxOverrunFlag);    // ������ɾ��
}

void LPUART4_IRQHandler(void)
{
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART4)) {
        // �����ж� 
        flexio_camera_uart_handler();

        gps_uart_callback();
    }

    LPUART_ClearStatusFlags(LPUART4, kLPUART_RxOverrunFlag);    // ������ɾ��
}

void LPUART5_IRQHandler(void)
{
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART5)) {
        // �����ж�
        camera_uart_handler();
    }

    LPUART_ClearStatusFlags(LPUART5, kLPUART_RxOverrunFlag);    // ������ɾ��
}

void LPUART6_IRQHandler(void)
{
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART6)) {
        // �����ж�

    }

    LPUART_ClearStatusFlags(LPUART6, kLPUART_RxOverrunFlag);    // ������ɾ��
}





void GPIO1_Combined_0_15_IRQHandler(void)
{
    if (exti_flag_get(B0)) {
        exti_flag_clear(B0);// ����жϱ�־λ
    }

}


void GPIO1_Combined_16_31_IRQHandler(void)
{
    wireless_module_spi_handler();
    if (exti_flag_get(B16)) {
        exti_flag_clear(B16); // ����жϱ�־λ
    }


}

void GPIO2_Combined_0_15_IRQHandler(void)
{
    flexio_camera_vsync_handler();

    if (exti_flag_get(C0)) {
        exti_flag_clear(C0);// ����жϱ�־λ
    }

}


void GPIO2_Combined_16_31_IRQHandler(void)
{
    if (exti_flag_get(C31)) {
        Keys[0].Key_State = 1;
        exti_flag_clear(C31); // ����жϱ�־λ
    }
    else
        if (exti_flag_get(C27)) {
            Keys[0].Key_State = 1;
            exti_flag_clear(C27); // ����жϱ�־λ
        }
        else
            if (exti_flag_get(C26)) {
                Keys[0].Key_State = 1;
                exti_flag_clear(C26); // ����жϱ�־λ
            }

}




void GPIO3_Combined_0_15_IRQHandler(void)
{

    if (exti_flag_get(D4)) {
        exti_flag_clear(D4);// ����жϱ�־λ
    }
}









/*
�жϺ������ƣ��������ö�Ӧ���ܵ��жϺ���
Sample usage:��ǰ���������ڶ�ʱ���ж�
void PIT_IRQHandler(void)
{
    //��������־λ
    __DSB();
}
�ǵý����жϺ������־λ
CTI0_ERROR_IRQHandler
CTI1_ERROR_IRQHandler
CORE_IRQHandler
FLEXRAM_IRQHandler
KPP_IRQHandler
TSC_DIG_IRQHandler
GPR_IRQ_IRQHandler
LCDIF_IRQHandler
CSI_IRQHandler
PXP_IRQHandler
WDOG2_IRQHandler
SNVS_HP_WRAPPER_IRQHandler
SNVS_HP_WRAPPER_TZ_IRQHandler
SNVS_LP_WRAPPER_IRQHandler
CSU_IRQHandler
DCP_IRQHandler
DCP_VMI_IRQHandler
Reserved68_IRQHandler
TRNG_IRQHandler
SJC_IRQHandler
BEE_IRQHandler
PMU_EVENT_IRQHandler
Reserved78_IRQHandler
TEMP_LOW_HIGH_IRQHandler
TEMP_PANIC_IRQHandler
USB_PHY1_IRQHandler
USB_PHY2_IRQHandler
ADC1_IRQHandler
ADC2_IRQHandler
DCDC_IRQHandler
Reserved86_IRQHandler
Reserved87_IRQHandler
GPIO1_INT0_IRQHandler
GPIO1_INT1_IRQHandler
GPIO1_INT2_IRQHandler
GPIO1_INT3_IRQHandler
GPIO1_INT4_IRQHandler
GPIO1_INT5_IRQHandler
GPIO1_INT6_IRQHandler
GPIO1_INT7_IRQHandler
GPIO1_Combined_0_15_IRQHandler
GPIO1_Combined_16_31_IRQHandler
GPIO2_Combined_0_15_IRQHandler
GPIO2_Combined_16_31_IRQHandler
GPIO3_Combined_0_15_IRQHandler
GPIO3_Combined_16_31_IRQHandler
GPIO4_Combined_0_15_IRQHandler
GPIO4_Combined_16_31_IRQHandler
GPIO5_Combined_0_15_IRQHandler
GPIO5_Combined_16_31_IRQHandler
WDOG1_IRQHandler
RTWDOG_IRQHandler
EWM_IRQHandler
CCM_1_IRQHandler
CCM_2_IRQHandler
GPC_IRQHandler
SRC_IRQHandler
Reserved115_IRQHandler
GPT1_IRQHandler
GPT2_IRQHandler
PWM1_0_IRQHandler
PWM1_1_IRQHandler
PWM1_2_IRQHandler
PWM1_3_IRQHandler
PWM1_FAULT_IRQHandler
SEMC_IRQHandler
USB_OTG2_IRQHandler
USB_OTG1_IRQHandler
XBAR1_IRQ_0_1_IRQHandler
XBAR1_IRQ_2_3_IRQHandler
ADC_ETC_IRQ0_IRQHandler
ADC_ETC_IRQ1_IRQHandler
ADC_ETC_IRQ2_IRQHandler
ADC_ETC_ERROR_IRQ_IRQHandler
PIT_IRQHandler
ACMP1_IRQHandler
ACMP2_IRQHandler
ACMP3_IRQHandler
ACMP4_IRQHandler
Reserved143_IRQHandler
Reserved144_IRQHandler
ENC1_IRQHandler
ENC2_IRQHandler
ENC3_IRQHandler
ENC4_IRQHandler
TMR1_IRQHandler
TMR2_IRQHandler
TMR3_IRQHandler
TMR4_IRQHandler
PWM2_0_IRQHandler
PWM2_1_IRQHandler
PWM2_2_IRQHandler
PWM2_3_IRQHandler
PWM2_FAULT_IRQHandler
PWM3_0_IRQHandler
PWM3_1_IRQHandler
PWM3_2_IRQHandler
PWM3_3_IRQHandler
PWM3_FAULT_IRQHandler
PWM4_0_IRQHandler
PWM4_1_IRQHandler
PWM4_2_IRQHandler
PWM4_3_IRQHandler
PWM4_FAULT_IRQHandler
Reserved171_IRQHandler
GPIO6_7_8_9_IRQHandler*/



