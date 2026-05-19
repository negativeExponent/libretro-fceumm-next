/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t dipsw;
} m577;

static SFORMAT StateRegs[] = {
	{ &m577.dipsw, 1, "DPSW" },
	{ 0 }
};

static void Sync(void) {
	uint8_t rd = (latch.addr & m577.dipsw & 0x30) ? FALSE : TRUE;
	uint8_t wr = FALSE;

	setprg16_access(0x8000, latch.addr, rd, wr);
	setprg16_access(0xC000, latch.addr, rd, wr);
	setchr8(latch.addr >> 1);
}

static void Power(void) {
	m577.dipsw = 0;
	Latch_Power();
}

static void Reset(void) {
	m577.dipsw += 0x10;
	Latch_RegReset();
}

void Mapper577_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
