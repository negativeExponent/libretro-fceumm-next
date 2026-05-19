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
	uint8_t prg, mode;
} m051;

static SFORMAT StateRegs[] = {
	{ &m051.prg, 1, "PREG" },
	{ &m051.mode, 1, "MODE" },
	{ 0 }
};

static void Sync(void) {
	if (iNESCart.submapper == 1) {
		setprg8(0x6000, (m051.prg << 1) | 0x23);
		if (m051.mode & 0x02)
			setprg32(0x8000, m051.prg >> 1);
		else {
			setprg16(0x8000, ((m051.prg >> 2) & 0x10) | ((m051.prg >> 1) & 0x08) | (m051.prg & 0x07));
			setprg16(0xC000, ((m051.prg >> 2) & 0x10) | ((m051.prg >> 1) & 0x08) | 0x07);
		}
	} else {
		if (m051.mode & 0x02) {
			setprg8(0x6000, ((m051.prg << 2) & 0x1C) | 0x23);
			setprg32(0x8000, m051.prg);
		} else {
			setprg8(0x6000, ((m051.prg << 2) & 0x10) | 0x2F);
			setprg16(0x8000, (m051.prg << 1) | (m051.prg >> 4));
			setprg16(0xC000, (m051.prg << 1) | 0x07);
		}
	}
	setchr8(0);
	setmirror(((m051.mode >> 4) & 1) ^ 1);
}

static DECLFW(WriteMode) {
	m051.mode = V;
	Sync();
}

static DECLFW(WritePRG) {
	m051.prg = V;
	Sync();
}

static void Power(void) {
	m051.prg = 0;
	m051.mode = 2;
	Sync();
	SetWriteHandler(0x6000, 0x7FFF, WriteMode);
	SetWriteHandler(0x8000, 0xFFFF, WritePRG);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
}

static void Reset(void) {
	m051.prg = 0;
	m051.mode = 2;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper051_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
	GameStateRestore = StateRestore;

	if ((ROM.chr.size == 8192) && (info->CHRRamSize == 8192)) {
		/* at least 1 variant has 8K CHR-ROM which should be treated as CHR-RAM */
		SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 1);
		AddExState(CHRptr[0], CHRsize[0], 0, "CHRR");
	}
}
