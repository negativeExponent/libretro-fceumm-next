#ifndef _VRC7_H
#define _VRC7_H

#include "mapinc.h"

typedef struct {
    uint8 prg[4];
    uint8 chr[8];
    uint8 mirr;

    void (*pwrap)(uint32 A, uint8 V);
    void (*cwrap)(uint32 A, uint32 V);
    void (*mwrap)(uint8 V);
} VRC7;

extern VRC7 vrc7;

void GenVRC7Power(void);
void GenVRC7Close(void);
void GenVRC7Restore(int version);
void FixVRC7PRG(void);
void FixVRC7CHR(void);
DECLFW(VRC7Write);

void GenVRC7_Init(CartInfo *info, uint32 A0, uint32 A1);

#endif /* _VRC7_H */