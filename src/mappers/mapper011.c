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
 *
 */

/* iNES Mapper 011 - The Color Dreams Mapper was a mapper used by the Color Dreams company. */
/* iNES Mapper 144, allocated for the game Death Race, describes a intentionally defective variant of the Color Dreams board (mapper 11). */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	setprg32(0x8000, latch.data);
	setchr8(latch.data >> 4);
}

static DECLFW(WriteLatch) {
	uint8_t reg = CartBR(A);
	latch.data = (reg & 0x01) | (V & reg & ~0x01);
	Sync();
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper011_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
}

void Mapper144_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
}
