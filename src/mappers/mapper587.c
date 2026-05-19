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
} m587;

static SFORMAT StateRegs[] = {
	{ m587.reg, 2, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	if (m587.reg[0] & 0x02) {
		setprg16(0x8000, (m587.reg[0] & ~0x07) | (m587.reg[1] & 0x07));
		setprg16(0xC000, (m587.reg[0] | 0x07));
	} else {
		if (m587.reg[0] & 0x80) {
			setprg32(0x8000, ((m587.reg[0] >> 1) & ~0x03) | (m587.reg[1] & 0x03));
		} else {
			setprg16(0x8000, (m587.reg[0] & ~0x07) | (m587.reg[1] & 0x07));
			setprg16(0xC000, (m587.reg[0] & ~0x07) | (m587.reg[1] & 0x07));
		}
	}
	setchr8(0);
	if (m587.reg[0] & 0x04) {
		setmirror(MI_0 + ((m587.reg[1] >> 4) & 0x01));
	} else {
		setmirror((m587.reg[0] & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	uint8_t idx = (m587.reg[0] >> 7) & 0x01;
	m587.reg[idx] = V;
	Sync();
}

static void Power(void) {
	m587.reg[0] = m587.reg[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper587_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
