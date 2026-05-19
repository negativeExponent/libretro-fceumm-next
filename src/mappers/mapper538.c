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

/* NES 2.0 Mapper 538 denotes the 60-1064-16L PCB, used for a
 * bootleg cartridge conversion named Super Soccer Champion
 * of the Konami FDS game Exciting Soccer.
 */

#include "mapinc.h"
#include "latch.h"
#include "fdssound.h"

/* this code emulates rom dump with wrong bank order */
static uint8_t prg_order_lut[16] = {
    0, 1, 2, 1, 3, 1, 4, 1,
    5, 5, 1, 1, 6, 6, 7, 7
};

static void Sync_alt(void) {
	setprg8(0x6000, (latch.data >> 1) | 8);
	setprg8(0x8000, prg_order_lut[latch.data & 0x0F]);
	setprg8(0xA000, 14);
	setprg8(0xC000, 7);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(1);
}

static void Sync(void) {
	setprg8(0x6000, latch.data | 1);
	setprg8(0x8000, (latch.data & 1) && (~latch.data & 8) ? 10 : (latch.data & ~1));
	setprg8(0xA000, 13);
	setprg8(0xC000, 14);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(MI_V);
}

static DECLFW(WriteLatch) {
	switch (A & 0xF000) {
	case 0xC000:
	case 0xD000:
		/* Latch only responds from $C000-$DFFF*/
		Latch_Write(A, V);
		break;
	}
}

static void Power(void) {
	FDSSound_Power();
	Latch_RegReset();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper538_Init(CartInfo *info) {
	if (info->CRC32 == 0xA8C6D77D) {
		Latch_Init(info, Sync_alt, NULL, 0, 0);
	} else {
		Latch_Init(info, Sync, NULL, FALSE, FALSE);
	}
	info->Power = Power;
}
