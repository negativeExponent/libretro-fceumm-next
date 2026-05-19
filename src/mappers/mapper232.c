/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
	uint8_t reg[2];
} m232;

static SFORMAT StateRegs[] = {
	{ m232.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t base = (m232.reg[0] >> 1) & 0x0C;

	if (iNESCart.submapper == 1) {
		base = ((base << 1) & 0x08) | ((base >> 1) & 0x04);
	}
	setprg16(0x8000, base | (m232.reg[1] & 0x03));
	setprg16(0xC000, base | 0x03);
	setchr8(0);
}

static DECLFW(WriteReg) {
	m232.reg[(A >> 14) & 0x01] = V;
	Sync();
}

static void Power(void) {
	m232.reg[0] = m232.reg[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper232_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, NULL);
}
