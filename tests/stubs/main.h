#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef enum {HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {uint32_t ISR,ICR,CR1,CR2,CR3,PRESC,BRR,RDR,TDR;} USART_TypeDef;
extern USART_TypeDef test_uart;
#define UART7 (&test_uart)
#define USART_ISR_ORE (1U<<0)
#define USART_ISR_FE (1U<<1)
#define USART_ISR_NE (1U<<2)
#define USART_ISR_PE (1U<<3)
#define USART_ISR_RXNE_RXFNE (1U<<4)
#define USART_ISR_TXE_TXFNF (1U<<5)
#define USART_ISR_TC (1U<<6)
#define USART_ICR_ORECF USART_ISR_ORE
#define USART_ICR_FECF USART_ISR_FE
#define USART_ICR_NECF USART_ISR_NE
#define USART_ICR_PECF USART_ISR_PE
#define USART_ICR_TCCF USART_ISR_TC
#define USART_CR1_TXEIE_TXFNFIE (1U<<0)
#define USART_CR1_TCIE (1U<<1)
#define USART_CR1_UE (1U<<2)
#define USART_CR1_TE (1U<<3)
#define USART_CR1_RE (1U<<4)
#define USART_CR1_RXNEIE_RXFNEIE (1U<<5)
#define USART_CR1_PEIE (1U<<6)
#define USART_CR3_EIE 1U
typedef struct {uint32_t Pin,Mode,Pull,Speed,Alternate;} GPIO_InitTypeDef;
typedef struct {uint32_t PeriphClockSelection,Usart234578ClockSelection;} RCC_PeriphCLKInitTypeDef;
#define GPIOA 0
#define GPIOB 1
#define GPIOD 2
#define GPIO_PIN_1 2U
#define GPIO_PIN_2 4U
#define GPIO_PIN_3 8U
#define GPIO_PIN_4 16U
#define GPIO_PIN_6 64U
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define GPIO_MODE_OUTPUT_PP 0
#define GPIO_MODE_AF_PP 1
#define GPIO_PULLUP 1
#define GPIO_PULLDOWN 2
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_AF11_UART7 11
#define RCC_PERIPHCLK_UART7 1
#define RCC_USART234578CLKSOURCE_D2PCLK1 1
#define UART7_IRQn 7
#define __HAL_RCC_GPIOD_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_UART7_CLK_ENABLE() ((void)0)
#define __HAL_RCC_UART7_FORCE_RESET() ((void)0)
#define __HAL_RCC_UART7_RELEASE_RESET() ((void)0)
#define __DMB() ((void)0)
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
#define HAL_GPIO_WritePin(a,b,c) ((void)0)
#define HAL_GPIO_Init(a,b) ((void)(b))
#define HAL_NVIC_SetPriority(a,b,c) ((void)0)
#define HAL_NVIC_EnableIRQ(a) ((void)0)
#define HAL_RCCEx_PeriphCLKConfig(x) ((void)(x),HAL_OK)
#define HAL_RCC_GetPCLK1Freq() 120000000U
#define Error_Handler() abort()
uint32_t HAL_GetTick(void);
#endif
