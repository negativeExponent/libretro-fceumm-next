/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint16_t bank = latch.addr >> 4;

	if (latch.addr & 0x100) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else {
		setprg32(0x8000, bank >> 1);
	}

	setchr8(latch.addr);
	setmirror(((latch.addr >> 9) & 0x01) ^ 0x01);
}

void Mapper576_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, FALSE);
	info->Reset = Latch_RegReset;
}
