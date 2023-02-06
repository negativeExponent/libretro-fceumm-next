/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

/*
 Mapper 228:
 Action 52 is highly uncommon in that its PRG ROM has a non-power-of-two ROM size: three 512 KiB PRG ROMs alongside one
 512 KiB CHR ROM.

 It is claimed that there are four 4-bit RAM locations at $4020-$4023, mirrored throughout $4020-$5FFF. This 16-bit RAM
 is definitely not present on either cartridge, Nestopia does not implement it at all, and neither cartridge ever writes
 to these addresses.
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint32 prgl, prgh, page = (latch.addr >> 7) & 0x3F;
	if ((page & 0x30) == 0x30)
		page -= 0x10;
	prgl = prgh = (page << 1) + (((latch.addr >> 6) & 1) & ((latch.addr >> 5) & 1));
	prgh += ((latch.addr >> 5) & 1) ^ 1;

	setmirror(((latch.addr >> 13) & 1) ^ 1);
	setprg16(0x8000, prgl);
	setprg16(0xc000, prgh);
	setchr8(((latch.data & 0x3) | ((latch.addr & 0xF) << 2)));
}

void Mapper228_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
}
