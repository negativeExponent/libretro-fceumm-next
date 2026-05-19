/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

#ifndef _JYASIC_H
#define _JYASIC_H

typedef struct __JYASIC {
	uint8_t mode[4];
	uint8_t prg[4];
	uint8_t mul[2];
	uint8_t adder;
	uint8_t test;
	uint8_t latch[2];
	uint16_t chr[8];
	uint16_t nt[4];
	struct irq {
		uint8_t control;
		uint8_t enable;
		uint8_t prescaler;
		uint8_t counter;
		uint8_t xor;
	} irq;
} JYASIC;

extern JYASIC jyasic;

extern uint8_t JYASIC_CPUWriteHandlersSet;
extern writefunc JYASIC_cpuWrite[0x10000];

DECLFR(JYASIC_ReadALU_DIP);
DECLFW(JYASIC_WriteALU);
DECLFW(JYASIC_WritePRG);
DECLFW(JYASIC_WriteCHRLow);
DECLFW(JYASIC_WriteCHRHigh);
DECLFW(JYASIC_WriteNT);
DECLFW(JYASIC_WriteIRQ);
DECLFW(JYASIC_WriteMode);
DECLFW(JYASIC_trapCPUWrite);

void JYASIC_restoreWriteHandlers(void);
void JYASIC_RegReset(void);
void JYASIC_Reset(void);
void JYASIC_Close(void);
void JYASIC_Power(void);
void JYASIC_Init(CartInfo *info, int extended_mirr);

void JYASIC_SyncPRG(void);
void JYASIC_SyncCHR(void);
void JYASIC_SyncWRAM(void);
void JYASIC_SyncMirror(void);

extern void (*JYASIC_pwrap)(uint16_t A, uint32_t V);
extern void (*JYASIC_wwrap)(uint16_t A, uint32_t V);
extern void (*JYASIC_cwrap)(uint16_t A, uint32_t V);
extern void (*JYASIC_mwrap)(uint16_t A, uint32_t V);

#endif /* _JYASIC_H */
