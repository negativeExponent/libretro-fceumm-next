
#ifndef _TXC_H
#define _TXC_H

typedef struct {
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

void GenTXCPower(void);
void TXCRegReset(void);
void TXCStateRestore(int version);
DECLFR(TXC_CMDRead);
DECLFW(TXC_CMDWrite);

void GenTXC_Init(CartInfo *info, void (*proc)(void));

#endif /* _TXC_H */