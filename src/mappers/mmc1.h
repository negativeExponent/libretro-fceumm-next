#ifndef _MMC1_H
#define _MMC1_H

#include "mapinc.h"

typedef enum {
    MMC1A,
    MMC1B
} MMC1Type;

typedef struct __MMC1 {
    uint8 regs[4];
    uint8 buffer;
    uint8 shift;
} MMC1;

extern MMC1 mmc1;

uint32 MMC1_GetPRGBank(int index);
uint32 MMC1_GetCHRBank(int index);

DECLFW(MMC1_Write);

void MMC1_Power(void);
void MMC1_Close(void);
void MMC1_Restore(int version);
void MMC1_Reset(void);

void MMC1_Init(CartInfo *info, int wram, int saveram);

void MMC1_FixPRG(void);
void MMC1_FixCHR(void);
void MMC1_FixMIR(void);

extern void (*MMC1_pwrap)(uint32 A, uint8 V);
extern void (*MMC1_cwrap)(uint32 A, uint8 V);
extern void (*MMC1_mwrap)(uint8 V);
extern void (*MMC1_wwrap)(void);

#endif /* _MMC1_H */
