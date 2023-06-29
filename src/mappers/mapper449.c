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

static uint8 dipswitch;

static SFORMAT StateRegs[] = {
    { &dipswitch,  1, "DIPS" },
    { 0 }
};

static void Sync(void) {
	uint8 prgbank = ((latch.addr >> 2) & 0x1F) | ((latch.addr >> 3) & 0x20);
	if (~latch.addr & 0x080) {
		setprg16(0x8000, prgbank);
		setprg16(0xC000, prgbank | 7);
	} else {
		if (latch.addr & 0x001) {
			setprg32(0x8000, prgbank >> 1);
		} else {
			setprg16(0x8000, prgbank);
			setprg16(0xC000, prgbank);
		}
	}
	setchr8(latch.data);
    setmirror((((latch.addr >> 1) & 1) ^ 1));
}

static DECLFR(M449Read) {
	if (latch.addr & 0x200) {
		return CartBR(A | dipswitch);
	}
	return CartBR(A);
}

static void M449Reset(void) {
	dipswitch  = (dipswitch + 1) & 0xF;
	LatchHardReset();
}

void Mapper449_Init(CartInfo *info) {
	Latch_Init(info, Sync, M449Read, 0, 0);
	info->Reset = M449Reset;
	AddExState(StateRegs, ~0, 0, 0);
}
