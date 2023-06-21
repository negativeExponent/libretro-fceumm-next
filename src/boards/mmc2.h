#ifndef _MMC2_H
#define _MMC2_H

typedef struct MMC2 {
    uint8 prg;
    uint8 chr[4];
    uint8 latch[2];
    uint8 mirr;
} MMC2;

extern MMC2 mmc2;

void GenMMC2Power(void);
void GenMMC2Close(void);
void GenMMC2Reset(void);
void GenMMC2Restore(int version);
DECLFW(MMC2_Write);
void MMC2_Init(CartInfo *info, int wram, int battery);

void MMC2_FixPRG(void);
void MMC2_FixCHR(void);

extern void (*MMC2_pwrap)(uint32 A, uint8 V);
extern void (*MMC2_cwrap)(uint32 A, uint8 V);
extern void (*MMC2_mwrap)(uint8 V);

#endif /* _MMC2_H */