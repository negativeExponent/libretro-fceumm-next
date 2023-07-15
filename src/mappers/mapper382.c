/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 382 - denotes the 830928C circuit board,
 * used on a 512 KiB 5-in-1 and a 1 MiB 9-in-1 multicart containing
 * the BNROM game Journey to the West and Capcom/Konami UNROM games.
 */

#include "mapinc.h"
#include "latch.h"

static uint8 outerbank;

static SFORMAT StateRegs[] = {
	{ &outerbank, 1, "BANK"},

	{ 0 }
};

static void Sync(void) {
	if (!(outerbank & 0x20)) {
		outerbank = latch.addr & 0x3F;
	}
	switch ((outerbank >> 3) & 1) {
		case 1:
			/* bnrom */
			setprg32(0x8000, (outerbank<< 2) | (latch.data & 3));
			break;
		default:
			/* unrom */
			setprg16(0x8000, (outerbank << 3) | (latch.data & 7));
			setprg16(0xC000, (outerbank << 3) | 7);
			break;
	}
	setchr8(0);
	setmirror(((outerbank >> 4) & 1) ^ 1);
	/* FCEU_printf("inB[0]:%02x outB[1]:%02x mode:%02x mirr:%02x lock:%02x\n", latch.data, latch.addr, mode, mirr, lock); */
}

static void M382Reset() {
	outerbank = 0;
	Latch_RegReset();
}

void Mapper382_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 1);
	info->Reset = M382Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
