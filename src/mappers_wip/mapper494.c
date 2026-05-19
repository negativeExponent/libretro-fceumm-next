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
} m494;

static SFORMAT StateRegs[] = {
	{ &m494.dipsw, 1, "DPSW" },
	{ 0 }
};

static void Sync() {
	uint16_t bank = latch.addr >> 2;
	uint8_t rd = (!(latch.addr & 0x100) && ((latch.addr & 0x01) & (m494.dipsw & 0x01))) ? FALSE : TRUE;
	uint8_t wr = 0;

	if (latch.addr & 0x100) {
		if (latch.addr & 0x001) {
			setprg16_access(0x8000, bank, rd, wr);
			setprg16_access(0xC000, bank, rd, wr);
		} else
			setprg32_access(0x8000, (bank >> 1), rd, wr);
	} else {
		setprg16_access(0x8000, bank, rd, wr);
		setprg16_access(0xC000, (bank | 0x07), rd, wr);
	}
	setchr8(((latch.addr >> 1) & 0x08) | ((latch.addr >> 5) & 0x07));
	setmirror(((latch.addr >> 1) & 0x01) ^ 0x01);
}

static void Reset(void) {
	m494.dipsw++;
	Latch_RegReset();
}

static void Power(void) {
	m494.dipsw = 0;
	Latch_Power();
}

void Mapper494_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
