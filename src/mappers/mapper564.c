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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	setchr8(0);
	setprg32(0x8000, latch.data);
	switch ((latch.data >> 4) & 3) {
	case 0:
		setmirror(MI_V);
		break;
	case 1:
		setmirror(MI_H);
		break;
	case 2:
		setmirror(MI_0);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

static DECLFW(WriteLatch) {
	if (latch.data & 0x20) {
		if (latch.data & 0x08) {
			latch.data = (latch.data & ~0x17) | (V & 0x17);
		} else {
			latch.data = (latch.data & ~0x13) | (V & 0x13);
		}
	} else {
		latch.data = V;
	}
	Sync();
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper564_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
