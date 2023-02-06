/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2020
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
#include "../ines.h"

static uint8 unrom, openbus;

static SFORMAT StateRegs[] =
{
	{ &openbus, 1, "OPNB" },
	{ &unrom,   1, "UROM" },
	{ 0 }
};

static void Sync(void) {
	if (unrom) { /* Contra mode */
		setprg16(0x8000, (ROM_size & 0xC0) | (latch.data & 7));
		setprg16(0xC000, (ROM_size & 0xC0) | 7);
		setchr8(0);
		setmirror(MI_V);
	} else {
		uint8 bank = ((latch.addr & 0x300) >> 3) | (latch.addr & 0x1F);
		if (latch.addr & 0x400) {
			setmirror(MI_0);
		} else {
			setmirror(((latch.addr >> 13) & 1) ^ 1);
		}
		if (bank >= (ROM_size >> 1)) {
			openbus = 1;
		} else if (latch.addr & 0x800) {
			setprg16(0x8000, (bank << 1) | ((latch.addr >> 12) & 1));
			setprg16(0xC000, (bank << 1) | ((latch.addr >> 12) & 1));
		} else {
			setprg32(0x8000, bank);
		}
		setchr8(0);
	}
}

static DECLFR(M235Read) {
	if (openbus) {
		openbus = 0;
		return X.DB;
	}
	return CartBR(A);
}

static void M235Reset(void) {
	if ((ROM_size * 16384) & 0x20000) {
		unrom = (unrom + 1) & 1;
	}
	LatchHardReset();
}

void Mapper235_Init(CartInfo *info) {
	Latch_Init(info, Sync, M235Read, 0, 0);
	info->Reset = M235Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
