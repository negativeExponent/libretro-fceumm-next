/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
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
	uint8_t latch;
	uint16_t lastPPUAddr;
} m096;

static SFORMAT StateRegs[] = {
	{ &m096.latch, 1, "LATC" },
	{ &m096.lastPPUAddr, 2 | FCEUSTATE_RLSB, "LADR" },
	{ 0 }
};

static void Sync(void) {
	setmirror(MI_0);
	setprg32(0x8000, latch.data & 0x03);
	setchr4(0x0000, (latch.data & 0x04) | (m096.latch & 0x03));
	setchr4(0x1000, (latch.data & 0x04) | 0x03);
}

static void PPUIRQHook(uint32_t A) {
	uint16_t addr = A & 0x3000;
	if (!(m096.lastPPUAddr & 0x2000) && (addr & 0x2000)) {
		uint8_t bank = A >> 8;
		if (m096.latch != bank) {
			m096.latch = bank;
			setchr4(0x0000, (latch.data & 0x04) | (m096.latch & 0x03));
		}
	}
	m096.lastPPUAddr = addr;
}

static void Power(void) {
	memset(&m096, 0, sizeof(m096));
	Latch_Power();
}

void Mapper096_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Power = Power;
	PPU_hook = PPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
