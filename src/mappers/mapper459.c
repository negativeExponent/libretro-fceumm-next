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

static void Sync(void) {
	uint8 prg = latch.addr >> 5;
	uint8 chr = (latch.addr & 0x03) | ((latch.addr >> 2) & 0x04) | ((latch.addr >> 4) & 0x08);
    chr &= (latch.addr & 8 ? 0x0F : 0x08);
	if (latch.addr & 4) {
		setprg32(0x8000, prg);
	} else {
		setprg16(0x8000, prg << 1);
		setprg16(0xC000, (prg << 1) | 7);
	}
	setchr8(chr);
	setmirror(((latch.addr >> 8) & 1) ^ 1);
}

void Mapper459_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
    info->Reset = Latch_RegReset;
}
