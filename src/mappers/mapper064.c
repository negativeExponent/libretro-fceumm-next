/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2025 negativeExponent
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

/* Mapper 64 - 	Tengen 800032 Rambo-1
 * Mapper 158 -	Tengen 800037 (Alien Syndrome Unl)
*/

#include "mapinc.h"

#define IRQ_MODE_A12 0
#define IRQ_MODE_CPU 1

static uint8 prg[4], chr[8];
static uint8 cmd, mirr;
static uint8 IRQa;
static uint8 IRQPrescaler;
static uint8 IRQCount;
static uint8 IRQLatch;
static uint8 IRQLatchExtra;
static uint8 IRQMode;
static uint8 IRQA12;
static uint8 IRQFilter;
static uint8 IRQDelay;
static uint8 IRQReload;

static void (*M064_FixMIR)(void);

static SFORMAT StateRegs[] = {
	{ prg,            4, "PREG" },
	{ chr,            8, "CREG" },
	{ &cmd,           1, "CMDR" },
	{ &mirr,          1, "MIRR" },

	{ &IRQa,          1, "IRQA" },
	{ &IRQPrescaler,  1, "IQPR" },
	{ &IRQCount,      1, "IRQC" },
	{ &IRQLatch,      1, "IRQL" },
	{ &IRQLatchExtra, 1, "IQLE" },
	{ &IRQReload,     1, "IRQR" },
	{ &IRQMode,       1, "IRQM" },
	{ &IRQA12,        1, "IQ12" },
	{ &IRQFilter,     1, "IRQF" },
	{ &IRQDelay,      1, "IRQD" },
	
	{ 0 }
};

static void IRQClockCounter(void) {
	if (IRQCount == 0) {
		IRQCount = IRQLatch + (IRQReload ? IRQLatchExtra : 0);
		if ((IRQCount == 0) && IRQReload && IRQa) {
			IRQDelay = 1;
		}
	} else {
		IRQCount--;
		if ((IRQCount == 0) && IRQa) {
			IRQDelay = 1;
		}
	}
	IRQReload = FALSE;
}

/* NEWPPU Only irq timing */

static void newppu_CPUHook(int a) {
	while (a--) {
		if (IRQDelay) {
			IRQDelay--;
			if (IRQDelay == 0) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
		IRQPrescaler++;
		if (!(IRQPrescaler & 0x03) && (IRQMode == IRQ_MODE_CPU)) {
			IRQClockCounter();
		}

		if (IRQA12) {
			if (!IRQFilter && (IRQMode == IRQ_MODE_A12)) {
				IRQClockCounter();
			}
			IRQFilter = 16;
		} else if (IRQFilter) {
			IRQFilter--;
		}
	}
}

static void newppu_PPUHook(uint32 A) {
	IRQA12 = (A & 0x1000) >> 12;
}

/******************/

static void M064IRQHook(int a) {
	while (a--) {
		if (IRQDelay) {
			IRQDelay--;
			if (IRQDelay == 0) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
		IRQPrescaler++;
		if (!(IRQPrescaler & 0x03) && (IRQMode == IRQ_MODE_CPU)) {
			IRQClockCounter();
		}
	}
}

static void M064HBHook(void) {
	if (((IRQMode == IRQ_MODE_A12) && (scanline != 240)) /*&& (scanline != 240)*/) {
		IRQClockCounter();
	}
}

static void M064_FixPRG(void) {
	uint16 pswap = (cmd << 8) & 0x4000;

	setprg8(0x8000 ^ pswap, prg[0]);
	setprg8(0xA000,         prg[1]);
	setprg8(0xC000 ^ pswap, prg[2]);
	setprg8(0xE000,         prg[3]);
}

static void M064_FixCHR(void) {
	uint16 cswap = (cmd << 5) & 0x1000;

	if (cmd & 0x20) {
		setchr1(0x0000 ^ cswap, chr[0]);
		setchr1(0x0400 ^ cswap, chr[6]);
		setchr1(0x0800 ^ cswap, chr[1]);
		setchr1(0x0C00 ^ cswap, chr[7]);
	} else {
		setchr2(0x0000 ^ cswap, (chr[0] >> 1));
		setchr2(0x0800 ^ cswap, (chr[1] >> 1));
	}
	setchr1(0x1000 ^ cswap, chr[2]);
	setchr1(0x1400 ^ cswap, chr[3]);
	setchr1(0x1800 ^ cswap, chr[4]);
	setchr1(0x1C00 ^ cswap, chr[5]);
}

static void m064_FixMIR(void) {
	setmirror((mirr & 1) ^ 1);
}

static void m158_FixMIR(void) {
	if (cmd & 0x20) {
		setntamem(NTARAM + ((chr[0] >> 7) << 10), 1, 0);
		setntamem(NTARAM + ((chr[6] >> 7) << 10), 1, 1);
		setntamem(NTARAM + ((chr[1] >> 7) << 10), 1, 2);
		setntamem(NTARAM + ((chr[7] >> 7) << 10), 1, 3);
	} else {
		setntamem(NTARAM + ((chr[0] >> 7) << 10), 1, 0);
		setntamem(NTARAM + ((chr[0] >> 7) << 10), 1, 1);
		setntamem(NTARAM + ((chr[1] >> 7) << 10), 1, 2);
		setntamem(NTARAM + ((chr[1] >> 7) << 10), 1, 3);
	}
}

static int ppumode = -1;

static void CheckPPUMode(void) {
	if (ppumode != newppu) {
		if (newppu) {
			MapIRQHook = newppu_CPUHook;
			PPU_hook = newppu_PPUHook;
			GameHBIRQHook = NULL;
		} else {
			MapIRQHook = M064IRQHook;
			PPU_hook = NULL;
			GameHBIRQHook = M064HBHook;
		}
		ppumode = newppu;
	}
}

static DECLFW(M064Write) {
	uint8 index;

	CheckPPUMode();

	switch (A & 0xE001) {
	case 0x8000:
		cmd = V;
		M064_FixPRG();
		M064_FixCHR();
		M064_FixMIR();
		break;
	case 0x8001:
		index = cmd & 0x0F;
		switch (index) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
			chr[index] = V;
			M064_FixCHR();
			M064_FixMIR();
			break;
		case 0x06:
		case 0x07:
			prg[index & 0x01] = V;
			M064_FixPRG();
			break;
		case 0x08:
		case 0x09:
			chr[index - 2] = V;
			M064_FixCHR();
			M064_FixMIR();
			break;
		case 0x0F:
			prg[2] = V;
			M064_FixPRG();
			break;
		}
		break;
	case 0xA000:
		mirr = V;
		M064_FixMIR();
		break;
	case 0xC000:
		IRQLatch = V;
		break;
	case 0xC001:
		IRQMode = (V & 1) ? IRQ_MODE_CPU : IRQ_MODE_A12;
		IRQPrescaler = 0;
		IRQCount = 0;
		IRQReload = TRUE;
		IRQLatchExtra = IRQFilter ? 0 : 1;
		break;
	case 0xE000:
		IRQa = FALSE;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xE001:
		IRQa = TRUE;
		break;
	}
}

static void M064Power(void) {
	cmd = mirr = 0;

	prg[0] = 0;
	prg[1] = 1;
	prg[2] = ~1;
	prg[3] = ~0;

	chr[0] = 0;
	chr[1] = 1;
	chr[2] = 2;
	chr[3] = 3;
	chr[4] = 4;
	chr[5] = 5;
	chr[6] = 6;
	chr[7] = 7;

	M064_FixPRG();
	M064_FixCHR();
	M064_FixMIR();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, M064Write);
}

static void StateRestore(int version) {
	CheckPPUMode();
	M064_FixPRG();
	M064_FixCHR();
	M064_FixMIR();
}

void Mapper064_Init(CartInfo *info) {
	info->Power = M064Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	M064_FixMIR = m064_FixMIR;
}

void Mapper158_Init(CartInfo *info) {
	info->Power = M064Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	M064_FixMIR = m158_FixMIR;
}
