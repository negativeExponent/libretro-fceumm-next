/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* NES 2.0 Mapper 465 */
/* 310-in-1 (UNL) (92 劃面選關 310-in-1 王牌老四強) */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint8_t prg = ((latch.addr >> 2) & 0x1F) | ((latch.addr >> 5) & 0x20);

	if (latch.addr & 0x200) {
		/* unrom */
		setprg16(0x8000, (prg & ~0x07) | (latch.data & 0x07));
		setprg16(0xC000, prg | 0x07);
	} else {
		if (latch.addr & 0x01) {
			setprg32(0x8000, prg >> 1);
		} else {
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		}
	}
	setchr8(0);
	setmirror(((latch.addr >> 1) & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (latch.addr & 0x200) {
		/* unrom latch */
		latch.data = V;
	} else {
		latch.addr = A & 0xFFFF;
	}
	Sync();
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper465_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
