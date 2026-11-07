#ifndef __ENCODER_H
#define __ENCODER_H	


#include "zf_common_headfile.h"

typedef struct Encoder_Init
{
			float Distance_Once[4];
			float Speed[4];
			float Distance[4];
}Encoder_Init;

void ENCODER_Init(void);
void ENCODER_RevSample (void);


#endif



