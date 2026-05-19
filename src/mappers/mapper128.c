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

static struct {
	uint16_t reg;
} m128;

static SFORMAT StateRegs[] = {
	{ &m128.reg, 2 | FCEUSTATE_RLSB, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, (m128.reg >> 2) | (latch.data & 0x07));
	setprg16(0xC000, (m128.reg >> 2) | 0x07);
	setchr8(0);
	setmirror(((m128.reg >> 1) & 0x01) ^ 0x01);
}

static DECLFW(WriteLatch) {
	if (m128.reg < 0xF000) {
		m128.reg = A & 0xFFFF;
	}
	Latch_Write(A, V);
}

static void Reset(void) {
	m128.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	m128.reg = 0;
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper128_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
