#include "pga112.h"
#include "spi.h"

//const uint16_t gainBuffer[10]={ 0x2a01,0x2a11,0x2a21,0x2a31,
//						 0x2a41,0x2a51,0x2a61,0x2a71,0xE100,0xE1F1 };
const uint16_t gainBuffer[11]={ 0x2a00,0x2a10,0x2a20,0x2a30,
						 0x2a40,0x2a50,0x2a60,0x2a70,0xE100,0xE1F1,0x2a0c};
						 
void SPI_Transmit16(uint8_t pga, uint16_t data) 
{/* 发送一个16位的数据 */
//		if(pga == 0){
			PGA_A_CS_Enable();
			PGA_A_SCLK_LOW();
	 
			for (uint16_t i = 0; i < 16; i++) 
			{//发送16位数据
	 
				if (data & 0x8000) {//检查最高位
					PGA_A_SCLK_LOW();
					PGA_A_DIO_HIGH();
				}else{
					PGA_A_SCLK_LOW();
					PGA_A_DIO_LOW();
				}
					data <<= 1;  //数据左移一位
					PGA_A_SCLK_HIGH();//上升沿捕获数据
			}
	 
			PGA_A_DIO_LOW(); //通信结束，释放所有引脚
			PGA_A_SCLK_LOW();
			PGA_A_CS_Disable();
//	}else{
//			PGA_B_CS_Enable();
//			PGA_B_SCLK_LOW();
//	 
//			for (uint16_t i = 0; i < 16; i++) 
//			{//发送16位数据
//	 
//				if (data & 0x8000) {//检查最高位
//					PGA_B_SCLK_LOW();
//					PGA_B_DIO_HIGH();
//				}else{
//					PGA_B_SCLK_LOW();
//					PGA_B_DIO_LOW();
//				}
//					data <<= 1;  //数据左移一位
//					PGA_B_SCLK_HIGH();//上升沿捕获数据
//			}
//	 
//			PGA_B_DIO_LOW(); //通信结束，释放所有引脚
//			PGA_B_SCLK_LOW();
//			PGA_B_CS_Disable();
//	}

}
						 					 
						 
//定义函数控制增益放大倍数
void SetGain(uint8_t gain,uint8_t pga)
{
	uint8_t gtemp=0;
 
	switch(gain)
	{
		case 1://max 10vpp
			gtemp=0;
			break;
		case 2://max 5vpp
			gtemp=1;
			break;
		case 5://max 2vpp
			gtemp=2;
			break;
		case 10://max 1vpp
			gtemp=3;
			break;
		case 20://max 0.5vpp
			gtemp=4;
			break;
		case 50://max 0.2vpp
			gtemp=5;
			break;
		case 100://max 0.1vpp
			gtemp=6;
			break;
		case 200://max 0.05vpp
			gtemp=7;
			break;
		case 201://GND
			gtemp=10;
			break;
	}

		SPI_Transmit16(pga,gainBuffer[gtemp]);

}

