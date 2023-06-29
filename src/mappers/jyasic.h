#ifndef _JYASIC_H
#define _JYASIC_H

typedef struct JYASIC_t {
	uint8 mode[4];
	uint8 prg[4];
	uint8 mul[2];
	uint8 adder;
	uint8 test;
	uint8 latch[2];
	uint16 chr[8];
	uint16 nt[4];
	struct irq {
		uint8 control;
		uint8 enable;
		uint8 prescaler;
		uint8 counter;
		uint8 xor;
	} irq;
} JYASIC_t;

extern JYASIC_t jyasic;

extern uint8 cpuWriteHandlersSet;
extern writefunc cpuWriteHandlers[0x10000];

DECLFR(readALU_DIP);
DECLFW(trapCPUWrite);
DECLFW(writeALU);
DECLFW(writePRG);
DECLFW(writeCHRLow);
DECLFW(writeCHRHigh);
DECLFW(writeNT);
DECLFW(writeIRQ);
DECLFW(writeMode);

void JYASIC_restoreWriteHandlers(void);
void JYASICRegReset(void);
void GenJYASICReset(void);
void GenJYASICClose(void);
void GenJYASICRestore(int version);
void GenJYASICPower(void);
void JYASIC_Init(CartInfo * info, int extended_mirr);

void JYASIC_FixPRG(void);
void JYASIC_FixCHR(void);
void JYASIC_FixMIR(void);

extern void (*JYASIC_pwrap)(uint32 A, uint32 V);
extern void (*JYASIC_wwrap)(uint32 A, uint32 V);
extern void (*JYASIC_cwrap)(uint32 A, uint32 V);

extern uint32 (*JYASIC_GetPRGBank)(uint32 V);
extern uint32 (*JYASIC_GetCHRBank)(uint32 V);

#endif /* _JYASIC_H */
