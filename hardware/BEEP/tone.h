#ifndef __TONE_H
#define __TONE_H

#include "main.h"

#define TONE_DURATION_INFINITE  0xFFFFFFFFU

void Tone_TimerHandler();
void tone(uint32_t freq, uint32_t duration);
void noTone();

#endif