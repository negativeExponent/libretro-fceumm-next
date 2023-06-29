/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* Mapper 237 - "Teletubbies / Y2K" 420-in-1 pirate multicart.
 * Dipswitch settings:
 * 0: 42-in-1
 * 1: 5,000-in-1
 * 2: 420-in-1
 * 3: 10,000,000-in-1 (lol)
 */

#include "mapinc.h"
#include "latch.h"

static uint8 dipswitch;

static SFORMAT StateRegs[] =
{
	{ &dipswitch, 1, "DPSW" },
	{ 0 }
};

static void Sync(void) {
	uint8 bank = (latch.data & 0x07);
	uint8 base = ((latch.addr << 3) & 0x20) | (latch.data & 0x18);
	uint8 mode = (latch.data & 0xC0) >> 6;

	setchr8(0);
	setprg16(0x8000, base | (bank & ~(mode & 1)));
	setprg16(0xC000, base | ((mode & 0x02) ? (bank | (mode & 0x01)) : 0x07));
	setmirror(((latch.data & 0x20) >> 5) ^ 1);
}

static DECLFW(M237Write) {
	if (latch.addr & 0x02) {
		latch.data &= ~0x07;
		latch.data |= (V & 0x07);
		Sync();
	} else {
		LatchWrite(A, V);
	}
}

static DECLFR(M237Read) {
	if (latch.addr & 0x01) {
		return dipswitch;
	}
	return CartBR(A);
}

static void M237Reset() {
	dipswitch++;
	dipswitch &= 3;
	LatchHardReset();
}

static void M237Power(void) {
	LatchPower();
	SetWriteHandler(0x8000, 0xFFFF, M237Write);
}

void Mapper237_Init(CartInfo *info) {
#if 0
	/* The menu system used by this cart seems to be configurable as 4 different types:
	 * 0: 42-in-1
	 * 1: 5,000-in-1
	 * 2: 420-in-1
	 * 3: 10,000,000-in-1 (lol)
	 */
	dipswitch = 0;
	if ((info->CRC32) == 0x272709b9) /* Teletubbies Y2K (420-in-1) */
		dipswitch = 2;
#endif
	Latch_Init(info, Sync, M237Read, 0, 0);
	info->Power = M237Power;
	info->Reset = M237Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
