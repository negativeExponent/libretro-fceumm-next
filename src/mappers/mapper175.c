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
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2], mirror;
} m175;

static SFORMAT StateRegs[] = {
	{ m175.reg, 2, "REG" },
	{ &m175.mirror, 1, "MIRR" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, m175.reg[0]);
	setprg16(0xC000, m175.reg[0]);
	setchr8(m175.reg[0]);
	setmirror(((m175.mirror >> 2) & 0x01) ^ 0x01);
}

static DECLFR(ReadReg) {
	switch (A & 0xF000) {
	case 0xF000:
		if (m175.reg[0] != m175.reg[1]) {
			m175.reg[0] = m175.reg[1];
			Sync();
		}
		break;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	switch (A & 0xF000) {
	case 0x8000:
		m175.mirror = V;
		Sync();
		break;
	case 0xA000:
		m175.reg[1] = V;
		break;
	}
}

static void Power(void) {
	m175.reg[0] = m175.reg[1] = m175.mirror = 0;
	SetReadHandler(0x8000, 0xFFFF, ReadReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper175_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
