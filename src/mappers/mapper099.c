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

/* Mapper 99 is a simple mapper used by Vs. System games such as Vs. Super Mario
 * Bros. It is comparable to CNROM, but without bus conflicts. */

#include "mapinc.h"

static struct {
	uint8_t latch;
} m099;
static writefunc cpuwrite4016;

static SFORMAT StateRegs[] = {
	{ &m099.latch, 1, "LATC" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	if (ROM.prg.size == SIZE_24K) { /* Vs. Tetris */
		unsetcpu8(0x8000);
		setprg8(0xA000, 0);
		setprg8(0xC000, 1);
		setprg8(0xE000, 2);
	} else {
		setprg32(0x8000, 0);
	}
	if (ROM.prg.size == (40 * 1024)) {
		setprg8(0x8000, m099.latch & 4); /* Special for VS Gumshoe */
	}
	setchr8(m099.latch >> 2);
}

static DECLFW(Write4016) {
	m099.latch = V;
	Sync();
	cpuwrite4016(A, V);
}

static void Power(void) {
	memset(&m099, 0, sizeof(m099));
	Sync();
	cpuwrite4016 = GetWriteHandler(0x4016);
	SetWriteHandler(0x4016, 0x4016, Write4016);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper099_Init(CartInfo *info) {
	info->Power = Power;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
