#include "AT24C02.h"
#include "i2c.h"
#include <string.h>


//初始化AT24C02  I2C接口
void AT24C02_I2C_Init(void)
{
	
}


void AD24C_SDA_OUT(void)
{

}

void AD24C_SDA_IN(void)
{
	
}


//I2C开始
void AT24C_I2C_Start(void)
{

}	  
//I2C结束
void AT24C_I2C_Stop(void)
{
								   	
}
//等待应答ACK
u8  AT24C_IIC_Wait_Ack(void)
{
	 return 1;
} 

//不产生ACK应答		    
void AT24C_IIC_NAck(void)
{
	
}					 				     
		  
void AT24C_IIC_Send_Byte(u8 Data)
{                           
	 
} 	    


/* All entry points below are main-context only (HAL tick must be running). */
static HAL_StatusTypeDef last_status = HAL_OK;
static uint32_t failed_at, error_count;
static int retry_blocked;

HAL_StatusTypeDef AT24C_LastStatus(void) { return last_status; }
uint32_t AT24C_ErrorCount(void) { return error_count; }

static int retry_allowed(void)
{
    return !retry_blocked || (uint32_t)(HAL_GetTick() - failed_at) >= AT24C_RETRY_MS;
}
static HAL_StatusTypeDef record_status(HAL_StatusTypeDef status)
{
    last_status = status;
    if (status != HAL_OK) {
        failed_at = HAL_GetTick();
        retry_blocked = 1;
        ++error_count;
    } else retry_blocked = 0;
    return status;
}

u8 AT24C_IIC_Read_Byte(void)
{
    u8 data = 0;
    if (!retry_allowed()) return 0;
    HAL_StatusTypeDef status = record_status(HAL_I2C_Master_Receive(
        &hi2c1, Interface_ADDR, &data, sizeof(data), AT24C_TIMEOUT_MS));
    return status == HAL_OK ? data : 0;
}

HAL_StatusTypeDef AT24C_ReadBytes(u16 Addr, u8 *Data, u16 Size)
{
    if (!Data || !Size || Addr >= 256U || Size > 256U - Addr) return HAL_ERROR;
    if (!retry_allowed()) return HAL_BUSY;
    return record_status(HAL_I2C_Mem_Read(&hi2c1, ADDR_24LCxx_Read, Addr,
        I2C_MEMADD_SIZE_8BIT, Data, Size, AT24C_TIMEOUT_MS));
}

u8 AT24C_ReadOneByte(u16 Addr)
{
    u8 data = 0;
    return AT24C_ReadBytes(Addr, &data, 1) == HAL_OK ? data : 0;
}

HAL_StatusTypeDef AT24C_WriteOneByte(u16 Addr, u8 Data)
{
    if (Addr >= 256U) return HAL_ERROR;
    if (!retry_allowed()) return HAL_BUSY;
    return record_status(HAL_I2C_Mem_Write(&hi2c1, ADDR_24LCxx_Write, Addr,
        I2C_MEMADD_SIZE_8BIT, &Data, sizeof(Data), AT24C_TIMEOUT_MS));
}

uint32_t MCM_floatToIntBit(float x)
{
    uint32_t bits;
    memcpy(&bits, &x, sizeof(bits));
    return bits;
}
float IntBitTofloat(uint32_t x)
{
    float value;
    memcpy(&value, &x, sizeof(value));
    return value;
}
