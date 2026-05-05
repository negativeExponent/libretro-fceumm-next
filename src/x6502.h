/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _X6502H
#define _X6502H

#include "fceu.h"

/* 21.47~ MHz ÷ 12 = 1.789773 MHz */
#define NTSC_CLOCK_SPEED  1789772.7272727272727272

/* 26.60~ MHz ÷ 16 = 1.662607 MHz */
#define PAL_CLOCK_SPEED   1662607.125

/* 26.60~ MHz ÷ 15 = 1.773448 MHz */
#define DENDY_CLOCK_SPEED 1773447.467

#define NTSC_CPU (isDendy ? DENDY_CLOCK_SPEED : NTSC_CLOCK_SPEED)
#define PAL_CPU  PAL_CLOCK_SPEED

#define FCEU_IQEXT    0x001
#define FCEU_IQEXT2   0x002
/* ... */
#define FCEU_IQRESET  0x020
#define FCEU_IQNMI2   0x040 /* Delayed NMI, gets converted to *_IQNMI */
#define FCEU_IQNMI    0x080
#define FCEU_IQDPCM   0x100
#define FCEU_IQFCOUNT 0x200
#define FCEU_IQTEMP   0x800

#define N_FLAG  0x80
#define V_FLAG  0x40
#define U_FLAG  0x20
#define B_FLAG  0x10
#define D_FLAG  0x08
#define I_FLAG  0x04
#define Z_FLAG  0x02
#define C_FLAG  0x01

typedef struct __X6502 {
	int32_t tcount;		/* Temporary cycle counter */
	uint16_t PC;			/* I'll change this to uint32_t later... */
						/* I'll need to AND PC after increments to 0xFFFF */
						/* when I do, though.  Perhaps an IPC() macro? */
	uint16_t newPC;       /* used to override PC after at power on */
	uint8_t A, X, Y, S, P, mooPI;
	uint8_t jammed;

	int32_t count;
	uint32_t IRQlow;		/* Simulated IRQ pin held low(or is it high?).
						And other junk hooked on for speed reasons.*/
	uint8_t openbus;			/* Data bus "cache" for reads from certain areas */

	uint16_t opcode_PC;
	int32_t opcode_cycles;
	uint8_t opcode;
	uint8_t (*encryptOpcodeCB)(uint8_t opcode);

	#ifdef FCEUDEF_DEBUGGER
	int preexec;		/* Pre-exec'ing for debug breakpoints. */
	void (*CPUHook)(struct __X6502 *);
	uint8_t (*ReadHook)(struct __X6502 *, uint32_t);
	void (*WriteHook)(struct __X6502 *, uint32_t, uint8_t);
	#endif
} X6502;

#ifdef FCEUDEF_DEBUGGER
void X6502_Debug(void (*CPUHook)(X6502 *),
				 uint8_t (*ReadHook)(X6502 *, uint32_t),
				 void (*WriteHook)(X6502 *, uint32_t, uint8_t));

#endif

extern uint32_t timestamp;
extern uint32_t sound_timestamp;
extern X6502 cpu;

extern void (*MapIRQHook)(int a);

void X6502_Run(int32_t cycles);

void X6502_Init(void);
void X6502_Reset(void);
void X6502_Power(void);
void X6502_SetNewPC(uint16_t newPC);
void X6502_SetOpcodeEncryptCB(uint8_t (*callback)(uint8_t opcode));

void TriggerNMI(void);
void TriggerNMI2(void);

DECLFR(X6502_DMR);
DECLFW(X6502_DMW);

void X6502_IRQBegin(int w);
void X6502_IRQEnd(int w);

#endif
