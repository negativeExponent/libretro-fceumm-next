/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 337 - BMC-CTC-12IN1
 * 12-in-1 Game Card multicart
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_337
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint8_t prg = latch.data & 0x1F;

	setprg8(0x6000, 1);
	if (latch.data & 0x80) { /* UNROM */
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 0x07);
	} else {
		if (latch.data & 0x40) { /* NROM-256 */
			setprg32(0x8000, prg >> 1);
		} else { /* NROM-128 */
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		}
	}
	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], (latch.data >> 7) & 0x01);
	setchr8(0);
	setmirror(((latch.data >> 5) & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (A & 0x4000) {
		Latch_Write(A, (latch.data & ~0x07) | (V & 0x07));
	} else {
		Latch_Write(A, (latch.data & 0x07) | (V & ~0x07));
	}
}

static void Power(void) {
	Latch_Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper337_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
