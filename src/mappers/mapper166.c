/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 */

#include "mapinc.h"

static struct {
	uint8_t reg[4];
} m166;

static SFORMAT StateRegs[] = {
	{ m166.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t base = ((m166.reg[0] ^ m166.reg[1]) & 0x10) << 1;
	uint16_t bank = (m166.reg[2] ^ m166.reg[3]) & 0x1f;

	if (m166.reg[1] & 0x08) {
		bank &= 0xFE;
		setprg16(0x8000, base + bank + 0);
		setprg16(0xC000, base + bank + 1);
	} else {
		if (m166.reg[1] & 0x04) {
			setprg16(0x8000, 0x1F);
			setprg16(0xC000, base + bank);
		} else {
			setprg16(0x8000, base + bank);
			setprg16(0xC000, 0x07);
		}
	}
	setchr8(0);
}

static DECLFW(WriteReg) {
	m166.reg[(A >> 13) & 0x03] = V;
	Sync();
}

static void Power(void) {
	memset(&m166, 0, sizeof(m166));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper166_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8 * 1024;
	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	}
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
}
