/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022-2025-2026 negativeExponent
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

static const uint8_t dipswlut[5] = {
	0, 0x10, 0x30, 0x70, 0xF0
};

static uint8_t dipsw;

static void Sync(void) {
	uint8_t bank = latch.addr >> 1;

	if (!(latch.addr & 0x100) && (latch.addr & dipswlut[dipsw])) {
		unsetcpu16(0xC000);
	} else {
		if (latch.addr & 0x2000) { /* NROM-256 */
			setprg32(0x8000, bank >> 1);
		} else { /* NROM-128 */
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
	setchr8(latch.data);
	setmirror((latch.addr & 0x01) ^ 0x01);
}

static void Reset(void) {
	dipsw++;
	if (dipsw > 4) {
		dipsw = 0;
	}
	Sync();
}

static void Power(void) {
	dipsw = 0;
	Latch_Power();
}

void Mapper414_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Reset = Reset;
	info->Power = Power;
}
