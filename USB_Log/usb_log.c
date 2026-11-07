/* Main-loop producer; nonblocking CDC transport. Never controls the vehicle. */
#include "main.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usb_log.h"
#include <stdio.h>
#include <string.h>

static USBD_HandleTypeDef device;
static uint8_t rx[64], tx[64];
static char queue[16][96];
static unsigned head, tail;
static uint32_t dropped, last_status;
static uint8_t coding[7] = {0x00,0xC2,0x01,0,0,0,8};
static int started;
static int8_t cdc_init(void)
{
    USBD_CDC_SetTxBuffer(&device, tx, 0);
    USBD_CDC_SetRxBuffer(&device, rx);
    return USBD_OK;
}
static int8_t cdc_deinit(void) { return USBD_OK; }
static int8_t control(uint8_t cmd, uint8_t *p, uint16_t n)
{
    if (n == 7 && cmd == CDC_SET_LINE_CODING) memcpy(coding, p, 7);
    if (n == 7 && cmd == CDC_GET_LINE_CODING) memcpy(p, coding, 7);
    return USBD_OK;
}
static int8_t receive(uint8_t *p, uint32_t *n)
{
    /* CDC OUT intentionally ignored: logs cannot inject motor commands. */
    (void)p; (void)n;
    USBD_CDC_SetRxBuffer(&device, rx);
    USBD_CDC_ReceivePacket(&device);
    return USBD_OK;
}
static int8_t complete(uint8_t *p, uint32_t *n, uint8_t ep)
{ (void)p; (void)n; (void)ep; return USBD_OK; }
static USBD_CDC_ItfTypeDef callbacks = {cdc_init, cdc_deinit, control, receive, complete};

void USB_Log_Event(const char *event, const char *detail)
{
    unsigned next = (head + 1U) % 16U;
    if (next == tail) { ++dropped; return; }
    snprintf(queue[head], sizeof(queue[head]), "%lu %s %.48s\r\n",
             (unsigned long)HAL_GetTick(), event, detail);
    head = next;
}

void USB_Log_Init(void)
{
    if (USBD_Init(&device, &VCP_Desc, 0) != USBD_OK) return;
    if (USBD_RegisterClass(&device, &USBD_CDC) != USBD_OK) return;
    if (USBD_CDC_RegisterInterface(&device, &callbacks) != USBD_OK) return;
    if (USBD_Start(&device) != USBD_OK) return;
    started = 1;
    USB_Log_Event("BOOT", "UART7 115200 8N1 PWM_OFF");
}

void USB_Log_Poll(void)
{
    static unsigned offset;
    uint32_t now = HAL_GetTick();
    if (!started) return;
    if ((uint32_t)(now - last_status) >= 1000U) {
        char status[48];
        last_status = now;
        snprintf(status, sizeof(status), "PWM_OFF dropped=%lu", (unsigned long)dropped);
        USB_Log_Event("ALIVE", status);
    }
    /* Protect class pointer against reset/deconfigure IRQ. No waiting inside. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    USBD_CDC_HandleTypeDef *cdc = (USBD_CDC_HandleTypeDef *)device.pClassData;
    if (device.dev_state == USBD_STATE_CONFIGURED && cdc && !cdc->TxState && head != tail) {
        unsigned len = (unsigned)strlen(queue[tail]);
        unsigned n = len - offset;
        if (n > sizeof(tx)) n = sizeof(tx);
        memcpy(tx, queue[tail] + offset, n);
        USBD_CDC_SetTxBuffer(&device, tx, n);
        if (USBD_CDC_TransmitPacket(&device) == USBD_OK) {
            offset += n;
            if (offset == len) { offset = 0; tail = (tail + 1U) % 16U; }
        }
    }
    __set_PRIMASK(mask);
}
