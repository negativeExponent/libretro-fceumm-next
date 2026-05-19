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
	if ((latch.data & 0x1F) == 0x02) {
		setprg32(0x8000, latch.data >> 1);
	} else {
		setprg16(0x8000, latch.data);
		setprg16(0xC000, latch.data);
	}
	setchr8(latch.data);
	switch (latch.data >> 6) {
	case 0: setmirrorw(0, 0, 0, 1); break;
	case 1: setmirror(MI_V); break;
	case 2: setmirror(MI_H); break;
	case 3: setmirror(MI_1); break;
	}
}

void Mapper489_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Reset = Latch_RegReset;
}
