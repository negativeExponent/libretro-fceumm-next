/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
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
	uint8_t prg = latch.addr >> 7;
	uint8_t chr = latch.addr & 0x1F;
	uint8_t mirr = ((latch.addr >> 5) & 0x01) ^ 0x01;

	if (latch.addr & 0x40) {
		setprg32(0x8000, prg >> 1);
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	}
	setchr8(chr);
	setmirror(mirr);
}

static void Reset(void) {
	GetWriteHandler(0x0133)(0x0133, 0);
	Latch_RegReset();
}

void Mapper464_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Reset = Reset;
}
