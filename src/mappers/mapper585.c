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
} m585;

static SFORMAT StateRegs[] = {
	{ &m585.dipsw, 1, "DPSW" },
	{ 0 }
};

static void Sync(void) {
	uint8_t rd = !(latch.addr & m585.dipsw & 0x60);

	if (latch.addr & 0x01)
		setprg32_access(0x8000, latch.addr >> 2, rd, FALSE);
	else {
		setprg16_access(0x8000, latch.addr >> 1, rd, FALSE);
		setprg16_access(0xC000, latch.addr >> 1, rd, FALSE);
	}
	setchr8(latch.addr >> 1);
	setmirror((latch.addr >> 4) & 0x01);
}

static void Power(void) {
	m585.dipsw = 0;
	Latch_Power();
}

static void Reset(void) {
	m585.dipsw += 0x20;
	Latch_RegReset();
}

void Mapper585_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
