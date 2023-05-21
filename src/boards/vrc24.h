#ifndef _VRC24_H
#define _VRC24_H

enum VRC24Type {
	VRC2a = 1,	/* Mapper 22 */
	VRC2b,		/* Mapper 23 */
	VRC2c,		/* Mapper 25 */
	VRC4a,		/* Mapper 21 */
	VRC4b,		/* Mapper 25 */
	VRC4c,		/* Mapper 21 */
	VRC4d,		/* Mapper 25 */
	VRC4e,		/* Mapper 23 */
	VRC4f,		/* Mapper 23 */
	VRC4_559,
};

typedef struct {
    uint8 prgreg[2];
    uint8 chrreg[8];
    uint8 chrhi[8];
    uint8 cmd;
    uint8 mirr;

    void (*pwrap)(uint32 A, uint8 V);
    void (*cwrap)(uint32 A, uint32 V);
    void (*mwrap)(uint8 V);

} VRC24;

extern VRC24 vrc24;

void GenVRC24Power(void);
void GenVRC24Close(void);
void GenVRC24Restore(int version);
void FixVRC24PRG(void);
void FixVRC24CHR(void);
DECLFW(VRC24Write);

void GenVRC24_Init(CartInfo *info, enum VRC24Type type, int wram);

#endif /* _VRC24_H */
