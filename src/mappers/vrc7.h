#ifndef _VRC7_H
#define _VRC7_H

#include "mapinc.h"

typedef struct __VRC7 {
    uint8 prg[4];
    uint8 chr[8];
    uint8 mirr;
} VRC7;

extern VRC7 vrc7;

DECLFW(VRC7_Write);

void VRC7_Power(void);
void VRC7_Close(void);

void VRC7_Init(CartInfo *info, uint32 A0, uint32 A1);

void VRC7_FixPRG(void);
void VRC7_FixCHR(void);

extern void (*VRC7_pwrap)(uint32 A, uint8 V);
extern void (*VRC7_cwrap)(uint32 A, uint8 V);
extern void (*VRC7_mwrap)(uint8 V);

#endif /* _VRC7_H */
