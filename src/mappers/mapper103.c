/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 *
 * FDS Conversion - Doki Doki Panic
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t prg;
	uint8_t mirror;
	uint8_t ram_enabled;
} m103;

static SFORMAT StateRegs[] = {
	{ &m103.prg, 1, "PREG" },
	{ &m103.mirror, 1, "MIRR" },
	{ &m103.ram_enabled, 3, "RAME" },
	{ 0 }
};

static void Sync(void) {
	if (!m103.ram_enabled) {
		setprg8(0x6000, m103.prg);
	} else {
		setprg8r(0x10, 0x6000, 0);
	}
	setprg32(0x8000, 0x03);
	setchr8(0);
	setmirror(((m103.mirror >> 3) & 0x01) ^ 0x01);
}

static DECLFR(ReadRAM) {
	if (m103.ram_enabled && (A >= 0xB8000) && (A <= 0xD7FF)) {
		return WRAM[0x2000 + (A - 0xB800)];
	}
	return CartBR(A);
}

static DECLFW(WriteRAM) {
	/* Writes to RAM-mappable regions always go to RAM, even if RAM is disabled
	 * for reading (PRG-ROM area). */
	if ((A >= 0x6000) && (A <= 0x7FFF)) {
		WRAM[A - 0x6000] = V;
	} else if ((A >= 0xB8000) && (A <= 0xD7FF)) {
		WRAM[0x2000 + (A - 0xB800)] = V;
	}
}

static DECLFW(WritePRG) {
	m103.prg = V;
	Sync();
}

static DECLFW(WriteMirror) {
	m103.mirror = V;
	Sync();
}

static DECLFW(WriteRAMDisable) {
	m103.ram_enabled = (V & 0x10) == 0;
	Sync();
}

static void Power(void) {
	memset(&m103, 0, sizeof(m103));

	FDSSound_Power();
	Sync();

	SetReadHandler(0x6000, 0x7FFF, ReadRAM);
	SetWriteHandler(0x6000, 0x7FFF, WriteRAM);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xB800, 0xD7FF, WriteRAM);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0xE000, 0xEFFF, WriteMirror);
	SetWriteHandler(0xF000, 0xFFFF, WriteRAMDisable);
}

static void Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper103_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 16384;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
