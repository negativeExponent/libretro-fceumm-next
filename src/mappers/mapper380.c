/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2023
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

/* NES 2.0 Mapper 380 denotes the 970630C circuit board,
 * used on a 512 KiB multicart having 42 to 80,000 listed NROM and UNROM games. */

#include "mapinc.h"
#include "latch.h"

static uint8 dipswitch;
static uint8 isKN35A;

static SFORMAT StateRegs[] = {
   { &dipswitch, 1, "DPSW" },

   { 0 }
};

static void Sync(void) {
	if (latch.addr & 0x200) {
		if (latch.addr & 1) { /* NROM 128 */
			setprg16(0x8000, latch.addr >> 2);
			setprg16(0xC000, latch.addr >> 2);
		} else /* NROM-256 */
			setprg32(0x8000, latch.addr >> 3);
	} else { /* UxROM */
		setprg16(0x8000, latch.addr >> 2);
		setprg16(0xC000, (latch.addr >> 2) | 7 | (isKN35A && latch.addr & 0x100 ? 8 : 0));
	}

	SetupCartCHRMapping(0, CHRptr[0], 0x2000, !(latch.addr & 0x80));
	setchr8(0);
	setmirror(((latch.addr >> 1) & 1) ^ 1);
}

static DECLFR(M380Read) {
	if (latch.addr & 0x100 && !isKN35A) {
		A |= dipswitch;
	}
	return CartBR(A);
}

static void M380Reset(void) {
	dipswitch = (dipswitch + 1) & 0xF;
	LatchHardReset();
}

void Mapper380_Init(CartInfo *info) {
	Latch_Init(info, Sync, M380Read, 0, 0);
	isKN35A = info->iNES2 && info->submapper == 1;
	info->Reset = M380Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
