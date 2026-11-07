#ifndef TEST_LOG_USBD_CORE_H
#define TEST_LOG_USBD_CORE_H
#include <stdint.h>
#define USBD_OK 0
#define USBD_BUSY 1
#define USBD_STATE_CONFIGURED 3
typedef struct {void *pClassData;uint8_t dev_state;} USBD_HandleTypeDef;
extern int USBD_CDC;
int USBD_Init(USBD_HandleTypeDef *,void *,int);
int USBD_RegisterClass(USBD_HandleTypeDef *,void *);
int USBD_Start(USBD_HandleTypeDef *);
#endif
