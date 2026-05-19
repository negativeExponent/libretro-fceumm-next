/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
	uint8_t prg, mirror;
} m071;

static SFORMAT StateRegs[] = {
	{ &m071.prg, 1, "PREG" },
	{ &m071.mirror, 1, "MIRR" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, m071.prg);
	setprg16(0xC000, ~0);
	setchr8(0);
	/* Fire Hawk or submapper 1, otherwise hard-mirroring */
	if (m071.mirror) {
		setmirror(m071.mirror);
	}
}

static DECLFW(WriteReg) {
	switch (A & 0xF000) {
	case 0x9000:
		m071.mirror = MI_0 + ((V >> 4) & 0x01);
		Sync();
		break;
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		m071.prg = V;
		Sync();
		break;
	}
}

static void Power(void) {
	m071.prg = 0;
	m071.mirror = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x9000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper071_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
