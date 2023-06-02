#ifndef _MMC1_H
#define _MMC1_H

#include "mapinc.h"

typedef enum {
    MMC1A,
    MMC1B
} MMC1Type;

typedef struct {
    uint8 regs[4];
    uint8 buffer;
    uint8 shift;

    void (*pwrap)(uint32 A, uint8 V);
    void (*cwrap)(uint32 A, uint8 V);
    void (*mwrap)(uint8 V);
    void (*wwrap)(void);
} MMC1;

extern MMC1 mmc1;

uint32 MMC1GetPRGBank(int index);
uint32 MMC1GetCHRBank(int index);

void GenMMC1Power(void);
void GenMMC1Close(void);
void GenMMC1Restore(int version);
void MMC1RegReset(void);

void FixMMC1PRG(void);
void FixMMC1CHR(void);
void FixMMC1MIRRORING(void);

DECLFW(MMC1Write);

void GenMMC1_Init(CartInfo *info, MMC1Type type, int prg, int chr, int wram, int saveram);

#endif /* _MMC1_H */