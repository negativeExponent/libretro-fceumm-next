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

/* NOTE: Only loading Contra after a reset? RAM init also affects powerup */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint8_t prg = ((latch.addr >> 3) & 0x20) | ((latch.addr >> 2) & 0x1F);
	uint8_t cpuA14 = latch.addr & 0x01;
	uint8_t nrom = (latch.addr >> 7) & 0x01;
	uint8_t unrom = (latch.addr >> 8) & 0x01;

	setprg16(0x8000, ((prg & ~cpuA14) & ~(0x07 * unrom)) | (latch.data * unrom));
	setprg16(0xC000, ((prg | cpuA14) & ~(0x07 * !nrom * !unrom)) | (0x07 * !nrom * unrom));

	SetupCartCHRMapping(0, CHRptr[0], 0x2000, nrom ? 0 : 1);
	setchr8(0);

	setmirror(((latch.addr >> 1) & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (latch.addr & 0x100) {
		latch.data = V & 0x07;
		Sync();
	} else {
		Latch_Write(A, V);
	}
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper454_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
}
