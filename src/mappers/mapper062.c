/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
	uint8_t prg = (latch.addr & 0x40) | ((latch.addr >> 8) & 0x3F);
	uint8_t chr = ((latch.addr & 0x1F) << 2) | (latch.data & 0x03);
	uint8_t mirrorV = ((latch.addr >> 7) & 1) ^ 1;
	uint8_t A14 = (latch.addr & 0x20) == 0;

	/* FCEU_printf("%04x prg = %02x chr = %02x mirV = %d A14 = %d\n", latch.addr, prg, chr, mirrorV, A14); */

	setprg16(0x8000, prg & ~A14);
	setprg16(0xC000, prg | A14);
	setchr8(chr);
	setmirror(mirrorV);
}

void Mapper062_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Reset = Latch_RegReset;
}
