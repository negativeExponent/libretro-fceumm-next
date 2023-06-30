#ifndef _MMC3_H
#define _MMC3_H

typedef struct {
    uint8 cmd;
    uint8 opts;
    uint8 mirr;
    uint8 wram;
    uint8 reg[8];
    uint8 expregs[8]; /* extra regs, mostly for pirate/multicart carts */
} MMC3;

extern MMC3 mmc3;
extern int isRevB;

uint8 MMC3_GetPRGBank(int V);
uint8 MMC3_GetCHRBank(int V);

void MMC3_Power(void);
void MMC3_Reset(void);
void MMC3_Close(void);
void MMC3_IRQHBHook(void);
int  MMC3_WRAMWritable(uint32 A);
void MMC3_Init(CartInfo *info, int wram, int battery);

DECLFW(MMC3_CMDWrite); /* $ 0x8000 - 0xBFFF */
DECLFW(MMC3_IRQWrite); /* $ 0xC000 - 0xFFFF */
DECLFW(MMC3_Write); /* $ 0x8000 - 0xFFFF */

extern void (*MMC3_FixPRG)(void);
extern void (*MMC3_FixCHR)(void);
extern void (*MMC3_FixMIR)(void);

extern void (*MMC3_pwrap)(uint32 A, uint8 V);
extern void (*MMC3_cwrap)(uint32 A, uint8 V);

#endif /* _MMC3_H */
