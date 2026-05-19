/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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

/*
 * NES 2.0 Mapper 521 is used for 장두진 바둑교실: 입문편, commonly known as
 * Korean Igo. Its UNIF board name is DREAMTECH01, without prefix.
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m521;

static SFORMAT StateRegs[] = {
	{ &m521.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, m521.reg);
	setprg16(0xC000, PRG_BANK_COUNT(16) - 1);
	setchr8(0);
}

static DECLFW(WriteReg) {
	if ((A & 0x20) && !(A & 0x10)) {
		m521.reg = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m521, 0, sizeof(m521));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper521_Init(CartInfo *info) {
	GameStateRestore = StateRestore;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
