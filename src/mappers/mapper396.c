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
 *
 * NES 2.0 Mapper 396 - BMC-830752C
 * 1995 Super 8-in-1 (JY-050 rev0)
 * Super 8-in-1 Gold Card Series (JY-085)
 * Super 8-in-1 Gold Card Series (JY-086)
 * 2-in-1 (GN-51)
 */

#include "mapinc.h"
#include "latch.h"

static uint8 outerbank;

static SFORMAT StateRegs[] = {
	{ &outerbank, 1, "BANK" },

	{ 0 }
};

static void Sync(void) {
	if ((latch.addr & 0x6000) == 0x2000) {
		outerbank = latch.data;
	}
	setprg16(0x8000, (outerbank << 3) | (latch.data & 7));
	setprg16(0xC000, (outerbank << 3) | 7);
	setchr8(0);
	setmirror((outerbank & 0x60) ? 0 : 1);
}

static void M396Reset(void) {
	outerbank = 0;
	LatchHardReset();
}

void Mapper396_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Reset = M396Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
