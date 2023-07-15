#ifndef _KS202_H
#define _KS202_H

typedef struct __KS202 {
	uint8 reg[8];
	uint8 cmd;
} KS202;

extern KS202 ks202;

DECLFW(KS202_Write);

void KS202_Power(void);
void KS202_Close(void);
void KS202_Reset(void);
void KS202_Restore(int version);

void KS202_Init(CartInfo *info, void (*proc)(void), int wram, int battery);

#endif /* _KS202_H */
