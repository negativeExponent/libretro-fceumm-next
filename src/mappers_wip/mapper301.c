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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t half;
	uint8_t reg;
} m301;

static SFORMAT StateRegs[] = {
	{ &m301.half, 1, "REG2" },
	{ &m301.reg, 1, "REG1" },
	{ 0 }
};

static void Sync(void) {
	setprg16(0x8000, ((m301.half << 5) & 0x20) | ((m301.reg << 3) & 0x18) | (latch.data & 0x07));
	setprg16(0xC000, ((m301.half << 5) & 0x20) | ((m301.reg << 3) & 0x18) | 0x07);
	setchr8(0);
	setmirror(((m301.reg >> 2) & 1) ^ 1);
}

static DECLFW(WriteReg) {
	m301.reg = V;
	Sync();
}

static void Power(void) {
	m301.half = 0;
	m301.reg = 0;
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

static void Reset(void) {
	m301.half = (m301.half ^ 1);
	m301.reg = 0;
	RAM[0x100] = 0;
	Latch_RegReset();
}

void Mapper301_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
