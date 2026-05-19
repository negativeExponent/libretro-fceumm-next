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
	uint16_t bank = latch.addr >> 1 & 0x20 | latch.addr & 0x1F;

	if (latch.addr & 0x20) {
		if (latch.addr & 0x01) {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		} else
			setprg32(0x8000, bank >> 1);
	} else {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | 0x07);
	}
	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], (latch.addr & 0x20) ? 0 : 1);
	setchr8(0);
	setmirror(((latch.addr >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (!(latch.addr & 0x20 && ~A & 0x20)) {
		A = (A & ~0xC0) | (latch.addr & 0xC0);
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper581_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
