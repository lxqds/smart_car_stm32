/* Production IRQ/RX ring -> production legacy parser. No hardware access. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "vehicle_app.h"
USART_TypeDef test_uart;
static uint32_t tick;
static LegacyControlParser parser;
static unsigned calls,losses;
uint32_t HAL_GetTick(void){return tick;}
void USB_Log_Init(void){}
void USB_Log_Event(const char *e,const char *d){(void)e;(void)d;}
void USB_Log_Poll(void){}
void VehicleApp_LogPoll(void){}
static void accept(void *c,float speed,uint16_t servo,uint32_t arrived)
{(void)c;assert(speed==0.5f && servo==1500 && arrived==tick);calls++;}
void VehicleApp_Receive(uint8_t b,uint32_t arrived){Legacy_Feed(&parser,b,arrived,accept,0);}
void VehicleApp_RxLost(void){parser.used=0;losses++;}
#include "../Core/Src/comm_uart7.c"
static void inject(uint8_t b)
{test_uart.ISR=USART_ISR_RXNE_RXFNE;test_uart.RDR=b;UART7_IRQHandler();test_uart.ISR=0;}
int main(void)
{
    const uint8_t packet[]={0x24,0x42,1,11,0,0,0,0x3f,0xdc,5,0x92};
    Comm_UART7_Init();Legacy_Reset(&parser);tick=100;
    for(unsigned i=0;i<10;i++){inject(packet[i]);Comm_UART7_Poll();}
    assert(!calls);inject(packet[10]);Comm_UART7_Poll();assert(calls==1); /* no next-byte requirement */
    for(unsigned i=0;i<64;i++)for(unsigned j=0;j<11;j++)inject(packet[j]);
    uint32_t before=rx_tail;Comm_UART7_Poll();assert(((rx_tail-before)&(RX_SIZE-1))<=64);
    while(rx_head!=rx_tail)Comm_UART7_Poll();assert(calls==65 && tx_head==tx_tail);
    for(unsigned i=0;i<1100;i++)inject(0xff);
    Comm_UART7_Poll();assert(losses && parser.used==0);
    tick+=100;
    for(unsigned j=0;j<11;j++)inject(packet[j]);Comm_UART7_Poll();assert(calls==66);
    puts("PASS UART7 IRQ/ring -> legacy parser, final-byte dispatch, bounded bursts and overflow recovery");
}
