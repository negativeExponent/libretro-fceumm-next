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
} m588;

static SFORMAT StateRegs[] = {
	{ m588.reg, 2, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x6000, (0x08 | (m588.reg[1] & 0x07)));
	setprg32(0x8000, (m588.reg[0] >> 4) & 0x07);
	setchr8(m588.reg[0] & 0x0F);
	setmirror(((m588.reg[0] >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	m588.reg[(A >> 12) & 0x01] = V;
	Sync();
}

static void Power(void) {
	memset(&m588, 0, sizeof(m588));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xE000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper588_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
