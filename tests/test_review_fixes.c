/* Compile actual production modules with mocked hardware, not a parser replica. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"
USART_TypeDef test_uart;
static uint32_t tick, advance_tick, polls;
uint32_t HAL_GetTick(void) { return tick; }
void USB_Log_Init(void) {}
void USB_Log_Event(const char *e,const char *d) {(void)e;(void)d;}
void USB_Log_Poll(void) {++polls; tick+=advance_tick;}
#include "../Core/Src/comm_uart7.c"
#include "../Core/Inc/dac_safe.h"
#include "i2c.h"
I2C_HandleTypeDef hi2c1;
static unsigned i2c_calls;
static uint32_t timeout_seen;
static HAL_StatusTypeDef i2c_result;
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h,uint16_t d,uint16_t a,uint16_t m,uint8_t *b,uint16_t n,uint32_t t)
{(void)h;(void)d;(void)a;(void)m; ++i2c_calls; timeout_seen=t; memset(b,0x5a,n); return i2c_result;}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h,uint16_t d,uint16_t a,uint16_t m,uint8_t *b,uint16_t n,uint32_t t)
{return HAL_I2C_Mem_Read(h,d,a,m,b,n,t);}
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h,uint16_t d,uint8_t *b,uint16_t n,uint32_t t)
{return HAL_I2C_Mem_Read(h,d,0,0,b,n,t);}
#include "../hardware/AT24C02/AT24C02.c"

static void inject(const char *s)
{
    while(*s) {test_uart.ISR=USART_ISR_RXNE_RXFNE;test_uart.RDR=(uint8_t)*s++;UART7_IRQHandler();}
    test_uart.ISR=0;
}
static size_t drain(char *out)
{
    size_t n=0;
    while(tx_tail!=tx_head) {
        uint32_t before=tx_tail;
        test_uart.ISR=USART_ISR_TXE_TXFNF;
        UART7_IRQHandler();
        assert(tx_tail!=before);
        out[n++]=(char)test_uart.TDR;
    }
    test_uart.ISR=USART_ISR_TC;
    UART7_IRQHandler();
    test_uart.ISR=0;
    out[n]=0; return n;
}
static void finish_rx(void)
{unsigned guard=1000;while(rx_head!=rx_tail && guard--)Comm_UART7_Poll();assert(guard>0);}

int main(void)
{
    char out[4096];
    assert(DAC_CodeFromVolts(-1.0f)==0 && DAC_CodeFromVolts(0)==0);
    assert(DAC_CodeFromVolts(NAN)==0 && DAC_CodeFromVolts(INFINITY)==0);
    assert(DAC_CodeFromVolts(-INFINITY)==0 && DAC_CodeFromVolts(3.3f)==4095);
    assert(DAC_CodeFromVolts(100)==4095 && DAC_CodeFromVolts(1.65f)==2048);
    unsigned previous=0;
    for(unsigned i=0;i<=33000;++i){unsigned c=DAC_CodeFromVolts(i/10000.0f);assert(c>=previous&&c<=4095);previous=c;}
    puts("PASS DAC boundaries/nonfinite/monotonic 33001 samples");

    i2c_result=HAL_TIMEOUT;tick=UINT32_MAX-5U;
    assert(AT24C_ReadOneByte(0)==0 && AT24C_LastStatus()==HAL_TIMEOUT);
    assert(timeout_seen==AT24C_TIMEOUT_MS && i2c_calls==1);
    assert(AT24C_WriteOneByte(0,3)==HAL_BUSY && i2c_calls==1);
    tick+=AT24C_RETRY_MS-1U;
    assert(AT24C_WriteOneByte(0,3)==HAL_BUSY);
    ++tick;i2c_result=HAL_OK;
    assert(AT24C_WriteOneByte(0,3)==HAL_OK && i2c_calls==2);
    assert(AT24C_ErrorCount()==1 && AT24C_LastStatus()==HAL_OK);
    uint8_t b[10];assert(AT24C_ReadBytes(0,b,10)==HAL_OK && b[9]==0x5a);
    unsigned count=i2c_calls;assert(AT24C_ReadBytes(255,b,10)==HAL_ERROR&&i2c_calls==count);
    i2c_result=HAL_ERROR;assert(AT24C_IIC_Read_Byte()==0);
    assert(MCM_floatToIntBit(IntBitTofloat(0x3fc00000U))==0x3fc00000U);
    puts("PASS EEPROM timeout/status/cooldown/wrap/recovery");

    tick=1000;Comm_UART7_Init();
    inject("PING 00000001\nPING 00000002\n");finish_rx();
    drain(out);assert(strcmp(out,"PONG 00000001\nPONG 00000002\n")==0);
    /* Newline flood: no TX-ready signal; Poll still returns after four frames. */
    for(unsigned i=0;i<1023;++i)inject("\n");
    uint32_t before=rx_tail;polls=0;Comm_UART7_Poll();
    assert(((rx_tail-before)&(RX_SIZE-1))==POLL_FRAMES && polls<=POLL_FRAMES+1);
    unsigned calls=0;
    while(tx_free()>=TX_RESERVE && rx_head!=rx_tail){Comm_UART7_Poll();assert(++calls<1000);}
    assert(tx_free()<TX_RESERVE);before=rx_tail;Comm_UART7_Poll();assert(rx_tail==before);
    drain(out);assert(strlen(out)%4==0);
    while(rx_head!=rx_tail){Comm_UART7_Poll();drain(out);}
    puts("PASS bounded newline flood and TX backpressure without TX-ready");

    inject("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    before=rx_tail;Comm_UART7_Poll();assert(((rx_tail-before)&(RX_SIZE-1))==POLL_BYTES);
    finish_rx();inject("\n");finish_rx();
    inject("PING 00000003\n");advance_tick=1;before=rx_tail;Comm_UART7_Poll();assert(rx_tail==before);
    advance_tick=0;finish_rx();drain(out);assert(strcmp(out,"PONG 00000003\n")==0);
    puts("PASS byte and elapsed-time budgets");

    /* Exercise RX overflow and recovery, including ring index wrap. */
    for(unsigned i=0;i<1100;++i)inject("X");
    assert(rx_loss && rx_full);Comm_UART7_Poll();inject("\n");finish_rx();
    for(unsigned i=0;i<300;++i){inject("PING 00000004\n");finish_rx();drain(out);assert(strcmp(out,"PONG 00000004\n")==0);}
    for(uint32_t error=USART_ISR_ORE;error<=USART_ISR_PE;error<<=1){test_uart.ISR=error;UART7_IRQHandler();Comm_UART7_Poll();inject("\nPING 00000005\n");finish_rx();drain(out);assert(strcmp(out,"PONG 00000005\n")==0);}
    puts("PASS RX overflow/error injection/recovery and 300 wrap cycles");
    return 0;
}
