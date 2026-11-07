#ifndef __TONE_H
#define __TONE_H

#include "main.h"

#define TONE_DURATION_INFINITE  0xFFFFFFFFU

void Tone_TimerHandler();
void tone(uint32_t freq, uint32_t duration);
void noTone();
void Tone_ControlInit(void);
void Tone_ControlPoll(void);

#endif
