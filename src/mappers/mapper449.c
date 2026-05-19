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
 */

#include "mapinc.h"
#include "latch.h"

static uint8_t dipsw;

static void Sync(void) {
	uint32_t prg = ((latch.addr >> 3) & 0x20) | ((latch.addr >> 2) & 0x1F);

	if (latch.addr & 0x80) { /* NROM */
		if (latch.addr & 0x01) { /* NROM-256 */
			setprg32(0x8000, prg >> 1);
		} else { /* NROM-128 */
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		}
	} else { /* UNROM */
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 0x07);
	}

	setchr8(latch.data);
	setmirror((((latch.addr >> 1) & 0x01) ^ 0x01));
}

static DECLFR(ReadDIP) {
	if (latch.addr & 0x200) {
		A |= dipsw;
	}
	return CartBR(A);
}

static void Reset(void) {
	dipsw = (dipsw + 1) & 0xF;
	Latch_RegReset();
}

void Mapper449_Init(CartInfo *info) {
	Latch_Init(info, Sync, ReadDIP, FALSE, FALSE);
	info->Reset = Reset;
}
