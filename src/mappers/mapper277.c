/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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
	uint8_t prg = latch.data & 0x0F;

	if (latch.data & 0x08) {
		if (latch.data & 0x01) {
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		} else {
			setprg32(0x8000, prg >> 1);
		}
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 0x07);
	}
	setchr8(0);
	setmirror(((latch.data >> 4) & 0x01) ^ 0x01);
}

static DECLFW(LatchWrite) {
	if (!(latch.data & 0x20)) {
		Latch_Write(A, V);
	}
}

static void Power(void) {
	latch.addr = 0;
	latch.data = 0x08;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBROB);
	SetWriteHandler(0x8000, 0xFFFF, LatchWrite);
}

static void Reset(void) {
	latch.addr = 0;
	latch.data = 0x08;
	Sync();
}

void Mapper277_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
}
