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
	if (latch.addr & 0x20) { /* First chip: 256 KiB */
		setprg16(0x8000, ((latch.addr >> 1) & 0x08) | (latch.data & 0x07));
		setprg16(0xC000, ((latch.addr >> 1) & 0x08) | 0x07);
	} else { /* Second chip: 128 KiB (boot) */
		setprg16(0x8000, (0x10 | ((latch.addr >> 1) & 0x07)));
		setprg16(0xC000, (0x10 | ((latch.addr >> 1) & 0x07)));
	}
	setchr8(0);
	setmirror((latch.addr & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (!((latch.data & 0x08) && (~A & 0x08))) {
		A = latch.addr;
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper589_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
