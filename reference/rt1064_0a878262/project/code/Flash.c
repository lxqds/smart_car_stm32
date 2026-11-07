#include "flash.h"
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

bool flashSaveEnable = false;
//volatile FLASH_Status FLASHStatus = FLASH_BUSY; // flash状态

//临时存取数组
uint8_t ArrayParams_ForFlash[FLASH_SIZE] = 
{
		/*1*/       0x00,0x00,0x00,0x00, 	//[Flag] [舵机中值]
    /*2*/       0x00,0x00,0x00,0x00, 	//[Flag] [舵机左转阈值]
    /*3*/       0x00,0x00,0x00,0x00 	//[Flag] [舵机右转阈值]
};



/**
* @brief        存储系统配置（全部/ALL）
* @param        
* @ref          
* @author       Leo
* @note         
**/
void FLASH_SaveAllConfig(void)
{
    int i = 0;
    uint8_t buff[4] = {0};
    Bint16_Union bint16_Union;
    
    memset(ArrayParams_ForFlash,0,sizeof(ArrayParams_ForFlash));
    
    /*1*/       buff[0] = FLASH_DATA_OK; 
                bint16_Union.U16 = servoStr.thresholdMiddle;//舵机中值PWM
                buff[1] = bint16_Union.U8_Buff[0];
                buff[2] = bint16_Union.U8_Buff[1];
                memcpy(ArrayParams_ForFlash+(i++)*4,buff,4);
    
    /*2*/       buff[0] = FLASH_DATA_OK; 
                bint16_Union.U16 = servoStr.thresholdLeft;//舵机左转阈值
                buff[1] = bint16_Union.U8_Buff[0];
                buff[2] = bint16_Union.U8_Buff[1];
                memcpy(ArrayParams_ForFlash+(i++)*4,buff,4);
    
    /*3*/       buff[0] = FLASH_DATA_OK; 
                bint16_Union.U16 = servoStr.thresholdRight;//舵机右转阈值
                buff[1] = bint16_Union.U8_Buff[0];
                buff[2] = bint16_Union.U8_Buff[1];
                memcpy(ArrayParams_ForFlash+(i++)*4,buff,4);
                
    FLASH_WriteBuffToFlash(0,ArrayParams_ForFlash,sizeof(ArrayParams_ForFlash));
}

/**
* @brief        加载系统配置（全部/ALL）
* @param        
* @ref          
* @author       Leo
* @note         
**/
void FLASH_LoadAllConfig(void)
{
    int i = 0;
    Bint16_Union bint16_Union;
		uint8_t buff[4];
    
    FLASH_ReadFlashNBtye(0,ArrayParams_ForFlash,sizeof(ArrayParams_ForFlash));//读取Flash数据
    
    /*1*/		memcpy(buff,ArrayParams_ForFlash+(i++)*4,4);
                if(buff[0] == FLASH_DATA_OK)                //舵机中值PWM
                {
                    bint16_Union.U8_Buff[0] = buff[1];
                    bint16_Union.U8_Buff[1] = buff[2];
                    servoStr.thresholdMiddle = bint16_Union.U16;
                }
                else
                {
                    servoStr.thresholdMiddle = SERVO_PWM_MIDDLE;
                }
                
    /*2*/		memcpy(buff,ArrayParams_ForFlash+(i++)*4,4);
                if(buff[0] == FLASH_DATA_OK)                //舵机左转阈值
                {
                    bint16_Union.U8_Buff[0] = buff[1];
                    bint16_Union.U8_Buff[1] = buff[2];
                    servoStr.thresholdLeft = bint16_Union.U16;
                }
                else
                {
                    servoStr.thresholdLeft = SERVO_PWM_MAX_L;
                }
                
    /*3*/		memcpy(buff,ArrayParams_ForFlash+(i++)*4,4);
                if(buff[0] == FLASH_DATA_OK)                //舵机右转阈值
                {
                    bint16_Union.U8_Buff[0] = buff[1];
                    bint16_Union.U8_Buff[1] = buff[2];
                    servoStr.thresholdRight = bint16_Union.U16;
                }
                else
                {
                    servoStr.thresholdRight = SERVO_PWM_MAX_R;
                }
}


/**
* @brief        读取flash中指定位置的参数
* @param        *pch 存储该数据的变量,iaddr 该数据的偏移地址
* @ref          
* @author       Leo
* @note         
**/
void FLASH_ReadSpecifyParam(uint8_t *pch,int iaddr)
{
    FLASH_ReadFlashNBtye(0,ArrayParams_ForFlash,sizeof(ArrayParams_ForFlash));
}


/**
* @brief        想片内flash写数据
* @param        WriteAddress 偏移地址，pbuff 写入数据首地址，num pbuff的大小
* @ref          
* @author       Leo
* @note         
**/
void FLASH_WriteBuffToFlash(int WriteAddress,uint8_t * pbuff,int num)
{   
    int i = 0;
    uint16_t temp = 0;
		uint32_t buffer[3];
	
//		for(int i = 0; i < 3; i++) buffer[i] = (pbuff[i * 3] << 24) | (pbuff[i * 3 + 1] << 16) | (pbuff[i * 3 + 2] << 8) | (pbuff[i * 3 + 3]);
		for(int i = 0; i < 3; i++) buffer[i] = (pbuff[i * 4] << 24) | (pbuff[i * 4 + 1] << 16) | (pbuff[i * 4 + 2] << 8) | (pbuff[i * 4 + 3]);
		flash_write_page(64, 0, buffer, 3);
//    FLASH_UnlockBank1(); // 解锁flash
//    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

//    FLASHStatus = FLASH_BUSY;
//    FLASHStatus = FLASH_ErasePage(FLASH_ADDR_START + WriteAddress);
//    
//    if(FLASHStatus == FLASH_COMPLETE)
//    {
//        FLASHStatus = FLASH_BUSY;
//        for(i = 0;i < num;i ++)
//        {
//            temp = (uint16_t)pbuff[i];
//            FLASHStatus = FLASH_ProgramHalfWord(FLASH_ADDR_START + WriteAddress + i*2,temp);
//        }
//    }
//    FLASHStatus = FLASH_BUSY;
//    FLASH_LockBank1();
	
}


/**
* @brief        从flash中毒数据
* @param        ReadAddress 偏移地址，pbuff 存储数据数组，ReadNum pbuff大小
* @ref          
* @author       Leo
* @note         
**/
void FLASH_ReadFlashNBtye(int ReadAddress, uint8_t *pbuff, int ReadNum)
{
    int i;
    int iDataNum = 0;
    uint8_t cReadBuff[2*ReadNum];
		uint32_t buffer[4];
	
		
		flash_read_page(64,0,buffer,3);

		for(int i = 0; i < 3; i++){ 
			//buffer[i] = (pbuff[i * 3] << 24) | (pbuff[i * 3 + 1] << 16) | (pbuff[i * 3 + 2] << 8) | (pbuff[i * 3 + 3]);
			pbuff[i*4] 		= buffer[i]>>24;
			pbuff[i*4+1]	= buffer[i]>>16;
			pbuff[i*4+2] 	= buffer[i]>>8;
			pbuff[i*4+3] 	= buffer[i];
		}
//    ReadAddress = (unsigned int)FLASH_ADDR_START + ReadAddress;
//    while(iDataNum < ReadNum*2)
//    {
//            *(cReadBuff + iDataNum) = *(__IO uint8_t*) ReadAddress++;
//            iDataNum ++;
//    }
//    for(i = 0;i < ReadNum;i ++)
//    {
//        pbuff[i] = cReadBuff[i*2];
//    }
		
}


/**
* @brief        Flash存数据控制
* @param        
* @ref          
* @author       Leo
* @note         
**/
void FLASH_Handle(void)
{
    if(flashSaveEnable)
    {		
        FLASH_SaveAllConfig();
        flashSaveEnable = false;
    }
}



