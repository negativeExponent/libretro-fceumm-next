#ifndef _VRC6_H
#define _VRC6_H

typedef struct {
    uint8 prg[2];
    uint8 chr[8];
    uint8 mirr;

    void (*pwrap)(uint32 A, uint8 V);
    void (*cwrap)(uint32 A, uint32 V);
    void (*mwrap)(uint8 V);
} VRC6;

extern VRC6 vrc6;

void GenVRC6Power(void);
void GenVRC6Close(void);
void GenVRC6Restore(int version);
void FixVRC6PRG(void);
void FixVRC6CHR(void);
DECLFW(VRC6Write);

void GenVRC6_Init(CartInfo *info, uint32 A0, uint32 A1, int wram);

#endif /* _VRC6_H */
