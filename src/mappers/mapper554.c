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

/* Mapper 554 - Kaiser KS-7010
 * used for one of several cartridge conversions of the FDS version of
 * 悪魔城 Dracula, the Japanese version of Castlevania
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m554;

static SFORMAT StateRegs[] = {
	{ &m554.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x6000, m554.reg);
	setprg8(0x8000, 10);
	setprg8(0xA000, 11);
	setprg8(0xC000, 6);
	setprg8(0xE000, 7);
	setchr8(m554.reg);
	setmirror(MI_V);
}

static DECLFR(Read) {
	uint16_t adr = A & 0xFFFE;

	switch (A & 0xF000) {
	case 0xC000:
		if ((adr >= 0xCAB6) && (adr <= 0xCAD6)) {
			m554.reg = (adr >> 2) & 0x0F;
			Sync();
		}
		break;
	case 0xE000:
		if ((adr == 0xEBE2) || (adr == 0xEE32)) {
			m554.reg = (A >> 2) & 0x0F;
			Sync();
		}
		break;
	case 0xF000:
		if (adr == 0xFFFC) {
			m554.reg = (A >> 2) & 0x0F;
			Sync();
		}
		break;
	}

	return CartBR(A);
}

static void Power(void) {
	Sync();
	SetReadHandler(0x6000, 0xFFFF, Read);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper554_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
