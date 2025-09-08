#include "AT24C02.h"
#include "i2c.h"


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


u8 AT24C_IIC_Read_Byte(void)
{
	u8 Data;
	HAL_I2C_Master_Receive(&hi2c1,Interface_ADDR,&Data,sizeof(Data),HAL_MAX_DELAY);  
	return Data;
}


u8 AT24C_ReadOneByte(u16 Addr)
{				  
	u8 Data=0;		  	    																    
  AT24C_I2C_Stop();
	HAL_I2C_Mem_Read(&hi2c1,ADDR_24LCxx_Read,Addr,I2C_MEMADD_SIZE_8BIT,&Data,sizeof(Data),HAL_MAX_DELAY);
	return Data;
}

void AT24C_WriteOneByte(u16 Addr,u8 Data)
{				   	  	    																 
 	HAL_I2C_Mem_Write(&hi2c1,ADDR_24LCxx_Write,Addr,I2C_MEMADD_SIZE_8BIT,&Data,sizeof(Data),HAL_MAX_DELAY);
}

uint32_t MCM_floatToIntBit( float x )
{
  uint32_t * pInt;
  pInt = ( uint32_t * )( &x );
  return *pInt;
}

float IntBitTofloat( uint32_t x )
{
  float * pInt;
  pInt = ( float * )( &x );
  return *pInt;
}