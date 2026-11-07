#ifndef USB_LOG_H
#define USB_LOG_H
void USB_Log_Init(void);
void USB_Log_Poll(void);
void USB_Log_Event(const char *event, const char *detail);
#endif
