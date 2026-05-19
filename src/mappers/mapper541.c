/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 *
 */

/* LittleCom 160-in-1 multicart */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint8_t prg = latch.addr >> 2;

	if (latch.addr & 2) { /* NROM-128 */
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	} else { /* NROM=256 */
		setprg32(0x8000, prg >> 1);
	}
	setchr8(0);
	setmirror(latch.addr & 0x01);
}

static DECLFW(WriteLatch) {
	switch (A & 0xF000) {
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		Latch_Write(A, V);
		break;
	default:
		break;
	}
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper541_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
