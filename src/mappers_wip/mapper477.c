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
	if (latch.data & 0x0E) {
		setprg16(0x8000, ~0);
		setprg16(0xC000, latch.data);
	} else {
		setprg32(0x8000, latch.data >> 1);
	}
	setchr8(latch.data);
}

static DECLFW(WriteLatch) {
	/* Resistors placed on the first 128 KiB PRG ROM chip (banks 8-15) cause ROM
	 * to always win D0..D3. Otherwise, normal AND-type bus conflicts. */
	if (latch.data & 0x08) {
		V = ((CartBR(A) & 0x0F) | ((V & CartBR(A)) & ~0x0F));
	} else {
		V = (CartBR(A) & V);
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

static void Reset(void) {
	latch.data = 0;
	Latch_RegReset();
}

void Mapper477_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
}
