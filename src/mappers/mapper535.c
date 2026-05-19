/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
 *  Copyright (C) 2023-2025-2026 negativeExponent
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
 *
 * NES 2.0 Mapper 535 - UNL-LH53
 * FDS Conversion - Nazo no Murasamejō
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t reg, IRQa;
	int32_t IRQCount;
} m535;

static SFORMAT StateRegs[] = {
	{ &m535.reg, 1, "REG" },
	{ &m535.IRQa, 1, "IRQA" },
	{ &m535.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg8(0x6000, m535.reg);
	setprg32(0x8000, 0x03);
	setprg2r(0x10, 0xB800, 0);
	setprg2r(0x10, 0xC000, 1);
	setprg2r(0x10, 0xC800, 2);
	setprg2r(0x10, 0xD000, 3);
}

static DECLFW(WriteReg) {
	m535.reg = V;
	Sync();
}

static DECLFW(WriteIRQ) {
	m535.IRQa = V & 0x02;
	m535.IRQCount = 0;
	X6502_IRQEnd(FCEU_IQEXT);
}

static void CPUIRQHook(int a) {
	if (m535.IRQa) {
		m535.IRQCount += a;
		if (m535.IRQCount > 7560) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m535, 0, sizeof(m535));
	FDSSound_Power();
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xB800, 0xD7FF, CartBW);
	SetWriteHandler(0xE000, 0xEFFF, WriteIRQ);
	SetWriteHandler(0xF000, 0xFFFF, WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper535_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
