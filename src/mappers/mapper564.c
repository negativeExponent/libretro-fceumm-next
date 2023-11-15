/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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
	if (latch.data & 0x20) {
        setmirror(MI_0 + ((latch.data >> 4) & 1));
	} else if (latch.data & 0x10) {
		setmirror(((latch.data >> 4) & 1) ^ 1);
    }
}

static DECLFW(M564Write) {
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

static void M564Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, M564Write);
}

void Mapper564_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
    info->Power = M564Power;
	info->Reset = Latch_RegReset;
}
