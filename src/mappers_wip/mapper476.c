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
	uint8_t reg[4];
} m476;

static readfunc cpuread4016 = NULL;

static SFORMAT StateRegs[] = {
	{ &m476.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (m476.reg[2] & 0x04) {
		setprg32(0x8000, m476.reg[0]);
	} else {
		setprg16(0x8000, m476.reg[0]);
		setprg16(0xC000, m476.reg[0]);
	}
	setchr8(0);
	setmirror((m476.reg[2] & 0x01) ^ 0x01);
}

static DECLFR(ReadJoypadReg) {
	int i;
	uint8_t result = 0x40;

	GetWriteHandler(0x4016)(0x4016, 1);
	GetWriteHandler(0x4016)(0x4016, 0);

	if (A == 0x4016) {
		for (i = 0; i < 8; i++) {
			result <<= 1;
			result |= cpuread4016(0x4016) & 0x01;
		}
		result = ((result & 0x90) ? 0x01 : 0x00) | /* START/A */
				 ((result & 0x60) ? 0x02 : 0x00);  /* SELECT/B */
	} else if (A == 0x4017) {
		for (i = 0; i < 8; i++) {
			result <<= 1;
			result |= cpuread4016(0x4016) & 1;
		}
		result = ((result & 0x04) ? 0x08 : 0x00) | /* DOWN  */
				 ((result & 0x08) ? 0x02 : 0x00) | /* UP    */
				 ((result & 0x02) ? 0x04 : 0x00) | /* LEFT  */
				 ((result & 0x01) ? 0x10 : 0x00);   /* RIGHT */
	}
	return result;
}

static DECLFW(writeReg) {
	m476.reg[(A >> 8) & 0x03] = V;
	Sync();
}

static void Reset(void) {
	memset(&m476, 0, sizeof(m476));
	Sync();
}

static void Power(void) {
	memset(&m476, 0, sizeof(m476));
	Sync();

	cpuread4016 = GetReadHandler(0x4016);

	SetReadHandler(0x4016, 0x4017, ReadJoypadReg);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, writeReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper476_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
