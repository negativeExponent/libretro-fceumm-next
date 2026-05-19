/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* iNES Mapper 122 denotes the JY043 circuit board, a simpler bootleg variant of
 * Kaiser's KS-7058 circuit board. */

#include "mapinc.h"

static struct {
	uint8_t chr[2];
} m122;

static SFORMAT StateRegs[] = {
	{ m122.chr, 2, "CREG" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, 0);
	setchr4(0x0000, m122.chr[0]);
	setchr4(0x1000, m122.chr[1]);
}

static DECLFW(WriteReg) {
	m122.chr[A & 0x01] = V;
	Sync();
}

static void Power(void) {
	m122.chr[0] = m122.chr[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper122_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}

/* kept here as duplicate */
/* should emulate BBK Keyboard Famiclone's mapper */
void Mapper171_Init(CartInfo *info) {
	Mapper122_Init(info);
}
