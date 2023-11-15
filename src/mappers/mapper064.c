/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2024 negativeExponent
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

static uint8 cmd, mirr, reg[16];
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

static SFORMAT StateRegs[] = {
	{ reg,           16, "REGS" },
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

/* FIXME: Fix IRQ Timing to work for both non NEWPPU and NEWPPU with better timing. */

/* NEWPPU Only irq timing */

static void IRQClockCounter(void) {
	if (!IRQCount) {
		IRQCount = IRQLatch + (IRQReload ? IRQLatchExtra : 0);
		if (!IRQCount && IRQReload && IRQa) {
			IRQDelay = 1;
		}
	} else if (!(--IRQCount) && IRQa) {
		IRQDelay = 1;
	}
	IRQReload = FALSE;
}

static void newppu_CPUHook(int a) {
	while (a--) {
		if (IRQDelay) {
			if (IRQDelay && !(--IRQDelay)) {
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

static void RAMBO1IRQHook(int a) {
	static int32 smallcount;
	if (IRQMode) {
		smallcount += a;
		while (smallcount >= 4) {
			smallcount -= 4;
			IRQCount--;
			if (IRQCount == 0xFF)
				if (IRQa) X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void RAMBO1HBHook(void) {
	int sl = newppu ? newppu_get_scanline() : scanline;
	if ((!IRQMode) && (sl != 240)) {
		IRQReload = 0;
		IRQCount--;
		if (IRQCount == 0xFF) {
			if (IRQa) {
				IRQReload = 1;
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void Sync(void) {
	/*
	0000: R0: Select 2 (K=0) or 1 (K=1) KiB CHR bank at PPU $0000 (or $1000)
	0001: R1: Select 2 (K=0) or 1 (K=1) KiB CHR bank at PPU $0800 (or $1800)
	0010: R2: Select 1 KiB CHR bank at PPU $1000-$13FF (or $0000-$03FF)
	0011: R3: Select 1 KiB CHR bank at PPU $1400-$17FF (or $0400-$07FF)
	0100: R4: Select 1 KiB CHR bank at PPU $1800-$1BFF (or $0800-$0BFF)
	0101: R5: Select 1 KiB CHR bank at PPU $1C00-$1FFF (or $0C00-$0FFF)
	0110: R6: Select 8 KiB PRG ROM bank at $8000-$9FFF (or $C000-$DFFF)
	0111: R7: Select 8 KiB PRG ROM bank at $A000-$BFFF
	1000: R8: If K=1, Select 1 KiB CHR bank at PPU $0400 (or $1400)
	1001: R9: If K=1, Select 1 KiB CHR bank at PPU $0C00 (or $1C00)
	1111: RF: Select 8 KiB PRG ROM bank at $C000-$DFFF (or $8000-$9FFF)
	*/
	setprg8(0x8000, reg[6]);
	setprg8(0xA000, reg[7]);
	setprg8(0xC000, reg[15]);
	setprg8(0xE000, ~0);

	if (cmd & 0x20) {
		setchr1(0x0000, reg[0]);
		setchr1(0x0400, reg[8]);
		setchr1(0x0800, reg[1]);
		setchr1(0x0C00, reg[9]);
	} else {
		setchr1(0x0000, (reg[0] & 0xFE));
		setchr1(0x0400, (reg[0] & 0xFE) | 1);
		setchr1(0x0800, (reg[1] & 0xFE));
		setchr1(0x0C00, (reg[1] & 0xFE) | 1);
	}

	setchr1(0x1000, reg[2]);
	setchr1(0x1400, reg[3]);
	setchr1(0x1800, reg[4]);
	setchr1(0x1C00, reg[5]);

	if (iNESCart.mapper == 158) {
		if (cmd & 0x20) {
			setntamem(NTARAM + ((reg[0] >> 7) << 10), 1, 0);
			setntamem(NTARAM + ((reg[8] >> 7) << 10), 1, 1);
			setntamem(NTARAM + ((reg[1] >> 7) << 10), 1, 2);
			setntamem(NTARAM + ((reg[9] >> 7) << 10), 1, 3);
		} else {
			setntamem(NTARAM + ((reg[0] >> 7) << 10), 1, 0);
			setntamem(NTARAM + ((reg[0] >> 7) << 10), 1, 1);
			setntamem(NTARAM + ((reg[1] >> 7) << 10), 1, 2);
			setntamem(NTARAM + ((reg[1] >> 7) << 10), 1, 3);
		}
	} else {
		setmirror((mirr & 1) ^ 1);
	}

	if (newppu) {
		MapIRQHook = newppu_CPUHook;
		PPU_hook = newppu_PPUHook;
		GameHBIRQHook = NULL;
	} else {
		GameHBIRQHook = RAMBO1HBHook;
		MapIRQHook = RAMBO1IRQHook;
		PPU_hook = NULL;
	}
}

static DECLFW(M064Write) {
	switch (A & 0xF001) {
	case 0xA000:
		mirr = V;
		Sync();
		break;
	case 0x8000:
		cmd = V;
		break;
	case 0x8001:
		reg[cmd & 0x0F] = V;
		Sync();
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

	if (newppu) {
		switch (A & 0xF001) {
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
	} else {
		switch (A & 0xF001) {
		case 0xC000:
			IRQLatch = V;
			if (IRQReload == 1)
				IRQCount = IRQLatch;
			break;
		case 0xC001:
			IRQReload = 1;
			IRQCount = IRQLatch;
			IRQMode = V & 1;
			break;
		case 0xE000:
			IRQa = 0;
			X6502_IRQEnd(FCEU_IQEXT);
			if (IRQReload == 1)
				IRQCount = IRQLatch;
			break;
		case 0xE001:
			IRQa = 1;
			if (IRQReload == 1)
				IRQCount = IRQLatch;
			break;
		}
	}
}

static void M064Power(void) {
	cmd = mirr = 0;
	reg[0] = reg[1] = reg[2] = reg[3] = reg[4] = reg[5] = ~0;
	reg[6] = reg[7] = reg[8] = reg[9] = reg[10] = ~0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, M064Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper064_Init(CartInfo *info) {
	info->Power = M064Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
