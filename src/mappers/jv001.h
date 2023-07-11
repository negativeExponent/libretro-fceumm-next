
#ifndef _JV001_H
#define _JV001_H

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
} JV001;

extern JV001 jv001;

void GenJV001Power(void);
void JV001RegReset(void);
void JV001StateRestore(int version);
DECLFR(JV001_CMDRead);
DECLFW(JV001_CMDWrite);

void GenJV001_Init(CartInfo *info, void (*proc)(void));

#endif /* _JV001_H */
