#ifndef TEST_LOG_USBD_CDC_H
#define TEST_LOG_USBD_CDC_H
#include "usbd_core.h"
#define CDC_SET_LINE_CODING 0x20
#define CDC_GET_LINE_CODING 0x21
typedef struct {uint32_t TxState;} USBD_CDC_HandleTypeDef;
typedef struct {
    int8_t (*init)(void);int8_t (*deinit)(void);
    int8_t (*control)(uint8_t,uint8_t *,uint16_t);
    int8_t (*receive)(uint8_t *,uint32_t *);
    int8_t (*complete)(uint8_t *,uint32_t *,uint8_t);
} USBD_CDC_ItfTypeDef;
int USBD_CDC_RegisterInterface(USBD_HandleTypeDef *,USBD_CDC_ItfTypeDef *);
int USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *,uint8_t *,uint16_t);
int USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *,uint8_t *);
int USBD_CDC_ReceivePacket(USBD_HandleTypeDef *);
int USBD_CDC_TransmitPacket(USBD_HandleTypeDef *);
#endif
