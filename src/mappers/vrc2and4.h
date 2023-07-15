#ifndef _VRC24_H
#define _VRC24_H

typedef enum {
	VRC2a = 1,	/* Mapper 22 */
	VRC2b,		/* Mapper 23 */
	VRC2c,		/* Mapper 25 */
	VRC4a,		/* Mapper 21 */
	VRC4b,		/* Mapper 25 */
	VRC4c,		/* Mapper 21 */
	VRC4d,		/* Mapper 25 */
	VRC4e,		/* Mapper 23 */
	VRC4f,		/* Mapper 23 */
	VRC4_544,
	VRC4_559,
} VRC24Type;

enum {
	VRC2 = 0,
	VRC4,
};

typedef struct __VRC24 {
    uint8 prg[2];
    uint16 chr[8];
    uint8 cmd;
    uint8 mirr;
	uint8 latch; /* VRC2 $6000-$6FFF */
} VRC24;

extern VRC24 vrc24;

void VRC24_IRQCPUHook(int a);
void VRC24_Reset(void);
void VRC24_Power(void);
void VRC24_Close(void);
DECLFW(VRC24_Write);

void VRC24_Init(CartInfo *info, uint8 vrc4, uint32 A0, uint32 A1, int wram, int irqRepeated);

void VRC24_FixPRG(void);
void VRC24_FixCHR(void);
void VRC24_FixMIR(void);

extern void (*VRC24_pwrap)(uint32 A, uint8 V);
extern void (*VRC24_cwrap)(uint32 A, uint32 V);
extern void (*VRC24_mwrap)(uint8 V);
extern void (*VRC24_miscWrite)(uint32 A, uint8 V);

#endif /* _VRC24_H */
