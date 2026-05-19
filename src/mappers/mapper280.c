/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024-2026 negativeExponent
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

static struct {
	uint8_t unrom;
} m280;

static void Sync(void) {
	uint8_t nrom = (latch.addr & 0x80) != 0;
	uint8_t A14 = (latch.addr & 0x01) != 0;
	uint16_t prg = (latch.addr >> 2) & 0x1F;

	if (!m280.unrom && nrom)
		SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 1);

	if (m280.unrom) {
		setprg16(0x8000, 0x20 | (latch.data & 0x07));
		setprg16(0xC000, 0x20 | 0x07);
		setchr8(0);
		setmirror(MI_V);
	} else {
		setprg16(0x8000, prg & ~A14);
		setprg16(0xC000, (prg | A14) * nrom);
		setchr8(0);
		setmirror(((latch.addr >> 1) & 1) ^ 1);
	}
}

static void Reset(void) {
	m280.unrom = !m280.unrom;
	Latch_RegReset();
}

void Mapper280_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Reset = Reset;
	AddExState(&m280.unrom, 1, 0, "UNRM");
}
