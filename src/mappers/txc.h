
#ifndef _TXC_H
#define _TXC_H

typedef struct __TXC {
	uint8 accumulator;
	uint8 inverter;
	uint8 staging;
	uint8 output;
	uint8 increase;
	uint8 invert;
	uint8 A;
	uint8 B;
	uint8 X;
	uint8 Y;
} TXC;

extern TXC txc;

DECLFR(TXC_Read);
DECLFW(TXC_Write);

void TXC_Power(void);

void TXC_Init(CartInfo *info, void (*proc)(void));

#endif /* _TXC_H */
