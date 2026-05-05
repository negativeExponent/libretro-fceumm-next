/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
	setprg16(0x8000, (latch.addr << 3) | (latch.data & 0x07));
	setprg16(0xC000, (latch.addr << 3) | 0x07);
	setchr8(0);
	setmirror((latch.addr & 0x04) ? MI_V : MI_H);
}

static DECLFW(WriteLatch) {
	uint16_t newAddr = A & 0x0FFF;

	if (((A & 0xF000) == 0xF000) && (newAddr >= 0xF08) && (newAddr <= 0xF0F)) {
		latch.addr = newAddr;
	}
	latch.data = V;
	Sync();
}

static void Reset(void) {
	Latch_RegReset();
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper608_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
}
