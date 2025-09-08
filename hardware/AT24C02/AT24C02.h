#ifndef _AT24C02_H
#define _AT24C02_H	 
#include "main.h"

typedef uint8_t u8;
typedef uint16_t u16;

#define ADDR_24LCxx_Write 0xA0
#define ADDR_24LCxx_Read 0xA1

#define Interface_ADDR       0
#define BeepOn_ADDR          1
#define SaveSettings_ADDR    2
#define OTP_ADDR             3
#define VSET_H_ADDR          4
#define VSET_L_ADDR          5
#define ISET_H_ADDR          6
#define ISET_L_ADDR          7
#define OVP_H_ADDR           8
#define OVP_L_ADDR           9
#define OCP_H_ADDR           10
#define OCP_L_ADDR           11
#define DELAY_H_ADDR         12
#define DELAY_L_ADDR         13
#define AGAIN_H_ADDR         14
#define AGAIN_L_ADDR         15
#define CVCC_ADDR            16

void AT24C02_I2C_Init(void);
void AT24C_WriteOneByte(u16 Addr,u8 Data);
u8 AT24C_ReadOneByte(u16 Addr);
uint32_t MCM_floatToIntBit( float x );
float IntBitTofloat( uint32_t x );

#endif


