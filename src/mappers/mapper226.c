/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2009 qeed
 *  Copyright (C) 2019 Libretro Team
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

/* Updated 2019-07-12
 * Mapper 226 - Updated and combine UNIF Ghostbusters63in1 board (1.5 MB carts), different bank order
 * - some 1MB carts can switch game lists using Select
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m226;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &dipsw, 1, "RST" },
	{ m226.reg, 2, "LATC" },
	{ 0 }
};

static void Sync(void) {
	uint16_t bank = ((m226.reg[1] & 0x01) << 1) | ((m226.reg[0] >> 7) & 0x01);
	uint8_t prg = m226.reg[0] & 0x1F;

	/* 1536KiB PRG roms have different bank order */
	if ((ROM.prg.size == (1536 * 1024)) && (bank > 0)) {
		bank = (bank - 1);
	}

	bank = (bank << 5) | (m226.reg[0] & 0x1F);

	if (m226.reg[0] & 0x20) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else {
		bank >>= 1;
		setprg32(0x8000, bank);
	}

	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], !(m226.reg[1] & 0x02));
	setchr8(0);
	setmirror((m226.reg[0] >> 6) & 0x01);
}

static DECLFW(WriteReg) {
	m226.reg[A & 0x01] = V;
	Sync();
}

static void Power(void) {
	memset(&m226, 0, sizeof(m226));
	Sync();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void StateRestore(int version) {
	Sync();
}

static void Reset(void) {
	memset(&m226, 0, sizeof(m226));
	Sync();
}

void Mapper226_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
	GameStateRestore = StateRestore;
}
