/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2023-2025 negativeExponent
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
 * Oeka-kids board
 *
 * I might want to add some code to the mapper 96 PPU hook function
 * to not change CHR banks if the attribute table is being accessed,
 * if I make emulation a little more accurate in the future.
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8 chrlatch;
	uint16 lastPPUAddr;
} m096;

static SFORMAT StateRegs[] = {
	{ &m096.chrlatch, 1, "CHRL" },
	{ &m096.lastPPUAddr, 2, "LADR" },
	{ 0 }
};

static void Sync(void) {
	setmirror(MI_0);
	setprg32(0x8000, latch.data & 0x03);
	setchr4(0x0000, (latch.data & 0x04) | (m096.chrlatch & 0x03));
	setchr4(0x1000, (latch.data & 0x04) | 0x03);
}

static void M096PPUHook(uint32 A) {
	uint16 addr = A & 0x3000;
	if ((m096.lastPPUAddr != 0x2000) && (addr == 0x2000)) {
		uint8 chr = A >> 8;
		if (m096.chrlatch != chr) {
			m096.chrlatch = A >> 8;
			setchr4(0x0000, (latch.data & 0x04) | (m096.chrlatch & 0x03));
		}
	}
	m096.lastPPUAddr = addr;
}

void Mapper096_Init(CartInfo *info) {
	memset(&m096, 0, sizeof(m096));
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	PPU_hook = M096PPUHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
