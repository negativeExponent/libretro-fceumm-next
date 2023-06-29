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
extern int isRevB;

uint8 MMC3GetPRGBank(int V);
uint8 MMC3GetCHRBank(int V);

void GenMMC3Power(void);
void GenMMC3Restore(int version);
void MMC3RegReset(void);
void GenMMC3Close(void);
int  MMC3CanWriteToWRAM(void);
DECLFW(MMC3_CMDWrite);
DECLFW(MMC3_IRQWrite);
DECLFW(MMC3_Write);

void GenMMC3_Init(CartInfo *info, int wram, int battery);

extern void (*MMC3_FixPRG)(void);
extern void (*MMC3_FixCHR)(void);

extern void (*MMC3_pwrap)(uint32 A, uint8 V);
extern void (*MMC3_cwrap)(uint32 A, uint8 V);
extern void (*MMC3_mwrap)(uint8 V);

#endif /* _MMC3_H */
