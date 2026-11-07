/* Bring-up only: UART7 PB3 RX / PB4 TX, 115200 8N1.
 * Cooperative diagnostic module in main application. Not the vehicle protocol.
 * W25Q64 /CS on PD6 must also have a hardware pull-up during reset.
 */
#include "main.h"
#include <string.h>
#include <stdio.h>
#include "usb_log.h"

/* Single IRQ producer / main-loop consumer. Keep arrival timestamps so a
 * parser timeout does not depend on time spent sending a previous reply. */
#define RX_SIZE 1024U
static volatile struct { uint32_t tick; uint8_t byte; } rx_ring[RX_SIZE];
static volatile uint32_t rx_head, rx_tail, rx_loss;
static volatile uint32_t rx_ore, rx_fe, rx_ne, rx_pe, rx_full;

void UART7_IRQHandler(void)
{
    uint32_t status = UART7->ISR;
    uint32_t errors = status & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE);
    if (errors) {
        if (errors & USART_ISR_ORE) ++rx_ore;
        if (errors & USART_ISR_FE) ++rx_fe;
        if (errors & USART_ISR_NE) ++rx_ne;
        if (errors & USART_ISR_PE) ++rx_pe;
        UART7->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
        rx_loss = 1;
    }
    if (status & USART_ISR_RXNE_RXFNE) {
        uint8_t byte = (uint8_t)UART7->RDR;
        uint32_t next = (rx_head + 1U) & (RX_SIZE - 1U);
        if (next == rx_tail) { ++rx_full; rx_loss = 1; }
        else if (!rx_loss) {
            rx_ring[rx_head].byte = byte;
            rx_ring[rx_head].tick = HAL_GetTick();
            __DMB();
            rx_head = next;
        }
    }
}

static int send_text(const char *text)
{
    USB_Log_Event("TX_REQUEST", text);
    while (*text) {
        uint32_t start = HAL_GetTick();
        while (!(UART7->ISR & USART_ISR_TXE_TXFNF)) {
            if ((uint32_t)(HAL_GetTick() - start) > 20U) { USB_Log_Event("TX_TIMEOUT", "UART7" ); return 0; }
        }
        UART7->TDR = (uint8_t)*text++;
    }
    uint32_t done = HAL_GetTick();
    while (!(UART7->ISR & USART_ISR_TC)) {
        if ((uint32_t)(HAL_GetTick() - done) > 20U) { USB_Log_Event("TX_TIMEOUT", "UART7 TC"); return 0; }
    }
    USB_Log_Event("TX_COMPLETE", "UART7 hardware TC");
    return 1;
}

void Comm_UART7_Init(void)
{
    GPIO_InitTypeDef pin = {0};
    RCC_PeriphCLKInitTypeDef clk = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);
    pin.Pin = GPIO_PIN_6;
    pin.Mode = GPIO_MODE_OUTPUT_PP;
    pin.Pull = GPIO_PULLUP;
    pin.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &pin);

    /* No TIM init/start: PWM pins held low throughout this test. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
    pin.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    pin.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &pin);

    clk.PeriphClockSelection = RCC_PERIPHCLK_UART7;
    clk.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK) Error_Handler();
    __HAL_RCC_UART7_CLK_ENABLE();
    __HAL_RCC_UART7_FORCE_RESET();
    __HAL_RCC_UART7_RELEASE_RESET();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    pin.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    pin.Mode = GPIO_MODE_AF_PP;
    pin.Pull = GPIO_PULLUP;
    pin.Speed = GPIO_SPEED_FREQ_LOW;
    pin.Alternate = GPIO_AF11_UART7;
    HAL_GPIO_Init(GPIOB, &pin);

    UART7->CR1 = 0;
    UART7->CR2 = 0;
    UART7->CR3 = USART_CR3_EIE;
    UART7->PRESC = 0;
    UART7->BRR = (HAL_RCC_GetPCLK1Freq() + 57600U) / 115200U;
    UART7->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    HAL_NVIC_SetPriority(UART7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART7_IRQn);
    UART7->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                  USART_CR1_RXNEIE_RXFNEIE | USART_CR1_PEIE;

    USB_Log_Init();
    USB_Log_Event("UART_MODE", "IRQ_RX_RING1024 TEST1 PWM_OFF");
}

void Comm_UART7_Poll(void)
{
    static char line[32];
    static unsigned used;
    static int discard;
    static uint32_t last, stats_tick;
    /* Bounded service; return to the existing UI rather than owning main. */
    for (unsigned budget = 0; budget < RX_SIZE; ++budget) {
        USB_Log_Poll();
        uint32_t now = HAL_GetTick();
        if ((uint32_t)(now - stats_tick) >= 1000U) {
            char stats[80];
            snprintf(stats, sizeof(stats), "ORE=%lu FE=%lu NE=%lu PE=%lu FULL=%lu",
                     (unsigned long)rx_ore, (unsigned long)rx_fe, (unsigned long)rx_ne,
                     (unsigned long)rx_pe, (unsigned long)rx_full);
            USB_Log_Event("RX_STATS", stats);
            stats_tick = now;
        }
        if (rx_loss) {
            uint32_t mask = __get_PRIMASK();
            __disable_irq();
            rx_tail = rx_head;
            rx_loss = 0;
            __set_PRIMASK(mask);
            USB_Log_Event("RX_LOSS", "flush queue; discard until LF or idle");
            used = 0;
            discard = 1;
            last = now;
            continue;
        }
        if (rx_tail == rx_head) {
            if ((used || discard) && (uint32_t)(now - last) > 200U) {
                used = 0;
                discard = 0;
            }
            return;
        }
        __DMB();
        char c = (char)rx_ring[rx_tail].byte;
        uint32_t arrived = rx_ring[rx_tail].tick;
        __DMB();
        rx_tail = (rx_tail + 1U) & (RX_SIZE - 1U);
        if ((used || discard) && (uint32_t)(arrived - last) > 200U) {
            used = 0;
            discard = 0;
        }
        last = arrived;
        if (c == '\n') {
            line[used] = 0;
            USB_Log_Event(discard ? "RX_DISCARD" : "RX_LINE", line);
            if (!discard) {
                if (strcmp(line, "HELLO") == 0) {
                    send_text("HELLO UART7 TEST1 PWM_OFF\n");
                } else if (used == 13 && memcmp(line, "PING ", 5) == 0) {
                    int valid = 1;
                    for (unsigned i = 5; i < 13; ++i)
                        if (!((line[i] >= '0' && line[i] <= '9') ||
                              (line[i] >= 'A' && line[i] <= 'F'))) valid = 0;
                    if (valid) {
                        line[1] = 'O';
                        line[13] = '\n';
                        line[14] = 0;
                        send_text(line);
                    } else send_text("ERR\n");
                } else send_text("ERR\n");
            }
            used = 0;
            discard = 0;
        } else if (!discard) {
            if (used >= sizeof(line) - 1 || c < 32 || c > 126) {
                discard = 1;
                used = 0;
            } else line[used++] = c;
        }
    }
}
