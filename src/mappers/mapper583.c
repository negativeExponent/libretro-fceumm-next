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
	uint16_t bank = (latch.addr & ~0x0F) | ((latch.addr << 3) & 0x08);

	if (latch.addr & 0x20) {
		setprg32(0x8000, ((bank >> 1) & ~0x07) | (latch.data & 0x07));
		setmirror(MI_0 + ((latch.data >> 4) & 0x01));
	} else {
		if ((latch.addr & 0x10) && (latch.addr & 0x08)) {
			setprg16(0x8000, (bank & ~0x03) | (latch.data & 0x03));
			setprg16(0xC000, (bank | 0x03));
			setmirror(MI_H);
		} else {
			setprg16(0x8000, (bank & ~0x07) | (latch.data & 0x07));
			setprg16(0xC000, (bank | 0x07));
			setmirror(MI_V);
		}
	}
	setchr8(0);
}

static DECLFW(WriteLatch) {
	if (!((~latch.addr & 0x80) && (A & 0x80))) {
		A = latch.addr;
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper583_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
