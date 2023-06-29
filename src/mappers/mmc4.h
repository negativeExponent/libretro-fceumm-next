#ifndef _MMC4_H
#define _MMC4_H

typedef struct MMC4 {
    uint8 prg;
    uint8 chr[4];
    uint8 latch[2];
    uint8 mirr;
} MMC4;

extern MMC4 mmc4;

void GenMMC4Power(void);
void GenMMC4Close(void);
void GenMMC4Reset(void);
void GenMMC4Restore(int version);
DECLFW(MMC4_Write);
void MMC4_Init(CartInfo *info, int wram, int battery);

void MMC4_FixPRG(void);
void MMC4_FixCHR(void);

extern void (*MMC4_pwrap)(uint32 A, uint8 V);
extern void (*MMC4_cwrap)(uint32 A, uint8 V);
extern void (*MMC4_mwrap)(uint8 V);

#endif /* _MMC4_H */