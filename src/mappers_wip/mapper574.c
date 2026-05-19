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
	if (latch.addr & 0x40) { /* AOROM */
		setprg32(0x8000, latch.addr >> 2 & ~0x07 | latch.data & 0x07);
	} else if (latch.addr & 0x20) { /* ANROM */
		setprg32(0x8000, latch.addr >> 2 & ~0x03 | latch.data & 0x03);
	} else { /* NROM-256 */
		setprg32(0x8000, latch.addr >> 2 & ~0x03 | latch.addr & 0x03);
	}
	setchr8(0);
	if (latch.addr & 0x60) {
		setmirror(MI_0 + ((latch.data >> 4) & 0x01));
	} else {
		setmirror(((latch.data >> 4) & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteLatch) {
	if (latch.addr & 0x80) {
		A = (latch.addr & ~0x03) | (A & 0x03);
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper574_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
