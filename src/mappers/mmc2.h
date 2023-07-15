#ifndef _MMC2_H
#define _MMC2_H

typedef struct __MMC2 {
    uint8 prg;
    uint8 chr[4];
    uint8 latch[2];
    uint8 mirr;
} MMC2;

extern MMC2 mmc2;

DECLFW(MMC2_Write);

void MMC2_Power(void);
void MMC2_Close(void);
void MMC2_Reset(void);
void MMC2_Restore(int version);
void MMC2_Init(CartInfo *info, int wram, int battery);

void MMC2_FixPRG(void);
void MMC2_FixCHR(void);

extern void (*MMC2_pwrap)(uint32 A, uint8 V);
extern void (*MMC2_cwrap)(uint32 A, uint8 V);
extern void (*MMC2_mwrap)(uint8 V);

#endif /* _MMC2_H */
