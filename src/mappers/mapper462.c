/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024-2025-2026 negativeExponent
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
} m462;

static SFORMAT StateRegs[] = {
	{ &m462.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (m462.reg & 0x40) {
		setprg32(0x8000, ((m462.reg >> 3) & ~0x07) | (latch.data & 0x07));
		setmirror(MI_0 + ((latch.data >> 4) & 0x01));
	} else {
		setprg16(0x8000, ((m462.reg >> 2) & ~0x07) | (latch.data & 0x07));
		setprg16(0xC000, ((m462.reg >> 2) & ~0x07) | 0x07);
		setmirror((m462.reg >> 4) & 0x01);
	}
	setchr8(0);
}

static DECLFW(WriteReg) {
	m462.reg = V;
	Sync();
}

static void Reset(void) {
	memset(&m462, 0, sizeof(m462));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m462, 0, sizeof(m462));
	Latch_Power();
	SetWriteHandler(0xA000, 0xBFFF, WriteReg);
}

void Mapper462_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, TRUE, FALSE);
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
