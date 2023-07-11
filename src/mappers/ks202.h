#ifndef _KS202_H
#define _KS202_H

typedef struct ks202_t {
	uint8 reg[8];
	uint8 cmd;
} ks202_t;

extern ks202_t ks202;

void GenKS202Power(void);
void GenKS202Close(void);
void GenKS202Reset(void);
void GenKS202Restore(int version);

DECLFW(KS202_Write);

void KS202_Init(CartInfo *info, void (*proc)(void), int wram, int battery);

#endif /* _KS202_H */
