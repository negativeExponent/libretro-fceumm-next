/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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
#include "latch.h"

static uint8 dipsw;

static SFORMAT StateRegs[] = {
    { &dipsw,  1, "DPSW" },
    { 0 }
};

static void Sync(void) {
	uint8 bank = ((latch.addr >> 2) & 0x1F) | ((latch.addr >> 3) & 0x20);

	if (!(latch.addr & 0x080)) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | 0x07);
	} else {
		if (latch.addr & 0x001) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
	setchr8(latch.data);
    setmirror((((latch.addr >> 1) & 0x01) ^ 0x01));
}

static DECLFR(M449Read) {
	if (latch.addr & 0x200) {
		return CartBR(A | dipsw);
	}
	return CartBR(A);
}

static void M449Reset(void) {
	dipsw  = (dipsw + 1) & 0xF;
	Latch_RegReset();
}

void Mapper449_Init(CartInfo *info) {
	Latch_Init(info, Sync, M449Read, 0, 0);
	info->Reset = M449Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
