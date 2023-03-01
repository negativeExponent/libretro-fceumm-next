#ifndef _MMC3_H
#define _MMC3_H

typedef struct {
    uint8 cmd;
    uint8 opts;
    uint8 mirroring;
    uint8 wram;
    uint8 regs[8];
    uint8 expregs[8]; /* extra regs, mostly for pirate/multicart carts */
} MMC3;

extern MMC3 mmc3;

extern void (*pwrap)(uint32 A, uint8 V);
extern void (*cwrap)(uint32 A, uint8 V);
extern void (*mwrap)(uint8 V);

void GenMMC3Power(void);
void GenMMC3Restore(int version);
void MMC3RegReset(void);
void GenMMC3Close(void);
void FixMMC3PRG(int V);
void FixMMC3CHR(int V);
int  MMC3CanWriteToWRAM();
DECLFW(MMC3_CMDWrite);
DECLFW(MMC3_IRQWrite);

void GenMMC3_Init(CartInfo *info, int prg, int chr, int wram, int battery);

#endif /* _MMC3_H */
