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

static struct {
	uint8_t reg;
} m485;

static SFORMAT StateRegs[] = {
    { &m485.reg, 1, "REGS" },
    { 0 }
};

static void Sync(void) {
	if (latch.data & 0x20) {
		setprg16(0x8000, latch.data & 0x1F | m485.reg << 5);
		setprg16(0xC000, latch.data & 0x1F | m485.reg << 5);
	} else {
		setprg32(0x8000, latch.data >> 1 & 0x0F | m485.reg << 4);
	}
	setchr8(0);
	if (latch.data & 0x80) {
		setmirror((latch.data & 0x40) ? MI_1 : MI_H);
	} else {
		setmirror((latch.data & 0x40) ? MI_V : MI_0);
	}
}

static DECLFW(WriteReg) {
	m485.reg = V;
	Sync();
}

static void Reset(void) {
	m485.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	m485.reg = 0;
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper485_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
