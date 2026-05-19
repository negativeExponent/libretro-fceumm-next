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
 * NES 2.0 Mapper 400 is used for retroUSB's 8-BIT XMAS 2017 homebrew cartridge.
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m400;

static SFORMAT StateRegs[] = {
	{ &m400.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, (m400.reg[0] & ~0x07) | (latch.data & 0x07));
	setprg16(0xC000, (m400.reg[0] & ~0x07) | 0x07);
	setchr8(0);
	if (m400.reg[0] == 0x80) {
		setmirror(iNESCart.mirror);
	} else {
		setmirror(((m400.reg[0] >> 5) & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	m400.reg[0] = V;
	Sync();
}

static DECLFW(WriteLed) {
	m400.reg[1] = V;
}

static void Reset(void) {
	memset(&m400, 0x80, sizeof(m400));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m400, 0x80, sizeof(m400));
	Latch_Power();
	SetWriteHandler(0x7800, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xBFFF, WriteLed);
}

void Mapper400_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, TRUE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
