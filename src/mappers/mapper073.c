/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 *
 * Konami VRC-3
 *
 */

#include "mapinc.h"

static struct {
	uint8 prg;
	uint8 IRQx; /* autoenable */
	uint8 IRQm; /* mode */
	uint8 IRQa;
	uint16 IRQLatch, IRQCount;
} m073;

static SFORMAT StateRegs[] = {
	{ &m073.prg, 1, "PREG" },
	{ &m073.IRQa, 1, "IRQA" },
	{ &m073.IRQx, 1, "IRQX" },
	{ &m073.IRQm, 1, "IRQM" },
	{ &m073.IRQLatch, 2, "IRQL" },
	{ &m073.IRQCount, 2, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, m073.prg);
	setprg16(0xC000, ~0);
	setchr8(0);
}

static DECLFW(WriteReg) {
	switch (A & 0xF000) {
	case 0x8000:
		m073.IRQLatch &= 0xFFF0;
		m073.IRQLatch |= (V & 0x0F) << 0;
		break;
	case 0x9000:
		m073.IRQLatch &= 0xFF0F;
		m073.IRQLatch |= (V & 0x0F) << 4;
		break;
	case 0xA000:
		m073.IRQLatch &= 0xF0FF;
		m073.IRQLatch |= (V & 0x0F) << 8;
		break;
	case 0xB000:
		m073.IRQLatch &= 0x0FFF;
		m073.IRQLatch |= (V & 0x0F) << 12;
		break;
	case 0xC000:
		m073.IRQm = V & 0x04;
		m073.IRQx = V & 0x01;
		m073.IRQa = V & 0x02;
		if (m073.IRQa) {
			m073.IRQCount = m073.IRQLatch;
		}
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xD000:
		X6502_IRQEnd(FCEU_IQEXT);
		m073.IRQa = m073.IRQx;
		break;
	case 0xF000:
		m073.prg = V;
		Sync();
		break;
	}
}

static void CPUIRQHook(int a) {
	int32 i;

	if (m073.IRQa) {
		for (i = 0; i < a; i++) {
			uint32 IRQCountMask = m073.IRQm ? 0xFF : 0xFFFF;
			if ((m073.IRQCount & IRQCountMask) == IRQCountMask) {
				m073.IRQCount = m073.IRQLatch;
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				m073.IRQCount++;
			}
		}
	}
}

static void Power(void) {
	memset(&m073, 0, sizeof(m073));

	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper073_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
