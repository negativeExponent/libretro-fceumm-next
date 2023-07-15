#ifndef _FCEU_LATCH_H
#define _FCEU_LATCH_H

typedef struct __LATCH {
	uint16 addr;
    uint8 data;
} LATCH;

extern LATCH latch;

DECLFW(Latch_Write);

void Latch_Power(void);
void Latch_Close(void);
void Latch_RegReset();

void Latch_Init(CartInfo *info, void (*proc)(void), readfunc func, uint8 wram, uint8 busc);

#endif /* _FCEU_LATCH_H */
