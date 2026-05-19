/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022 NewRisingSun
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

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m294;

static SFORMAT StateRegs[] = {
	{ &m294.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t base = m294.reg << 3;

	setprg16(0x8000, base | (latch.data & 0x07));
	setprg16(0xC000, base | 0x07);
	setchr8(0);
	setmirror(((m294.reg >> 4) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m294.reg = V;
		Sync();
	}
}

static void Reset(void) {
	m294.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	m294.reg = 0;
	Latch_Power();
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
}

void Mapper294_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
