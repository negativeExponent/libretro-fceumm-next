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
 * FDS Conversion
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m120;

static SFORMAT StateRegs[] = {
	{ &m120.reg, 1, "REG" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x6000, m120.reg);
	setprg32(0x8000, 2);
	setchr8(0);
}

static DECLFW(WriteReg) {
	if ((A & 0xE1FF) == 0x41FF) {
		m120.reg = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m120, 0, sizeof(m120));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper120_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
