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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "mapinc.h"

/**** LEGACY MAPPER IMPLEMENTATION for Mapper 132 (UNL-22211) ****/

static struct {
	uint8_t reg[4];
} UNL22211;

static SFORMAT StateRegs[] = {
	{ UNL22211.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, (UNL22211.reg[2] >> 2) & 0x01);
	setchr8(UNL22211.reg[2] & 3);
}

static DECLFR(ReadReg) {
	return ((UNL22211.reg[1] ^ UNL22211.reg[2]) | 0x40);
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		UNL22211.reg[A & 0x03] = V;
		Sync();
	}
}

static void Power(void) {
	memset(&UNL22211, 0, sizeof(UNL22211));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetReadHandler(0x4100, 0x4100, ReadReg);
	SetWriteHandler(0x4100, 0x4FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void UNL22211_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
