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
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t lastnt;
} m514;

static SFORMAT StateRegs[] = {
	{ &m514.lastnt, 1, "LSNT" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg32(0x8000, latch.data);
	setchr4(0x0000, m514.lastnt);
	setchr4(0x1000, 1);
	setmirror(((latch.data >> 6) & 0x01) ^ 0x01);
}

static void PPUIRQHook(uint32_t A) {
	if ((A & 0x3000) == 0x2000) {
		uint32_t mask = (latch.data & 0x40) ? 0x02 : 0x01;
		uint32_t bank = A >> 10;
		if ((latch.data & 0x80) && (bank & mask)) {
			setchr4(0, 1);
			m514.lastnt = 1;
		} else {
			m514.lastnt = 0;
			setchr4(0, 0);
		}
	}
}

static void Reset(void) {
	Latch_RegReset();
}

static void Power(void) {
	m514.lastnt = 0;
	Latch_Power();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper514_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, TRUE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	PPU_hook = PPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
