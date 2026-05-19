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
	uint8_t rd = ((latch.addr & 0x10) && (PRG_BANK_COUNT(16) > 8)) ? FALSE : TRUE;
	uint8_t wr = FALSE;

	if (latch.addr & 0x02) {
		setprg16_access(0x8000, ((latch.addr << 3) & 0x10) | (latch.data & 0x0F), rd, wr);
		setprg16_access(0xC000, ((latch.addr << 3) & 0x10) | 0x0F, rd, wr);
	} else {
		setprg16_access(0x8000, ((latch.addr << 3) & 0x18) | (latch.data & 0x07), rd, wr);
		setprg16_access(0xC000, ((latch.addr << 3) & 0x18) | 0x07, rd, wr);
	}

	setchr8(0);
}

static DECLFW(WriteLatch) {
	V &= CartBR(A);
	if (latch.addr & 0x20) {
		A = latch.addr;
	}
	Latch_Write(A, V);
}

static void Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper573_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Latch_RegReset;
}
