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

/* NES 2.0 Mapper 378 denotes a circuit board with unknown ID, used for an
 * 8-in-1 512 KiB multicart containing one AxROM and two UNROM games. */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	if (latch.data & 0x20) {
		setprg16(0x8000, 0x10 | ((latch.data << 1) & 0x0E) | ((latch.data >> 3) & 0x01));
		setprg16(0xC000, 0x10 | ((latch.data << 1) & 0x0E) | 0x07);
        setmirror(((latch.data >> 2) & 0x01) ^ 0x01);
	} else {
		setprg32(0x8000, (latch.data & 0x07));
        setmirror(MI_0 + (latch.data >> 4) & 0x01);
	}
	setchr8(0);
}

void Mapper378_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
    info->Reset = Latch_RegReset;
}
