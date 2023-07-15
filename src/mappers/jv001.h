
#ifndef _JV001_H
#define _JV001_H

typedef struct __JV001 {
	uint8 accumulator;
	uint8 inverter;
	uint8 staging;
	uint8 output;
	uint8 increase;
	uint8 invert;
	uint8 A;
	uint8 B;
	uint8 X;
} JV001;

extern JV001 jv001;

void JV001_Power(void);
void JV001_Reset(void);

void JV001_Init(CartInfo *info, void (*proc)(void));

DECLFR(JV001_Read);
DECLFW(JV001_Write);

#endif /* _JV001_H */
