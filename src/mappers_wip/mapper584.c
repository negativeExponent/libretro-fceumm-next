/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m584;

static SFORMAT StateRegs[] = {
	{ m584.reg, 2, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	if (m584.reg[0] &0x20)
		setprg32(0x8000, m584.reg[0] >>1);
	else {
		setprg16(0x8000, m584.reg[0]);
		setprg16(0xC000, m584.reg[0]);
	}
	setchr8(m584.reg[1]);
	setmirror((m584.reg[1] >> 5) & 0x01);
}

static DECLFW(writeReg) {
	if (A & 0x100) {
		m584.reg[(A >> 13) & 0x01] = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m584, 0, sizeof(m584));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0x7FFF, writeReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper584_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
