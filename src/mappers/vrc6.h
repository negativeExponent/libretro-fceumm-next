#ifndef _VRC6_H
#define _VRC6_H

typedef struct __VRC6 {
    uint8 prg[2];
    uint8 chr[8];
    uint8 mirr;
} VRC6;

extern VRC6 vrc6;

DECLFW(VRC6_Write);

void VRC6_Power(void);
void VRC6_Close(void);
void VRC6_Restore(int version);

void VRC6_Init(CartInfo *info, uint32 A0, uint32 A1, int wram);

void VRC6_FixPRG(void);
void VRC6_FixCHR(void);

extern void (*VRC6_pwrap)(uint32 A, uint8 V);
extern void (*VRC6_cwrap)(uint32 A, uint8 V);
extern void (*VRC6_mwrap)(uint8 V);

#endif /* _VRC6_H */
