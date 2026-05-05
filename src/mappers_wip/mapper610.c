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

static struct {
	uint8_t reg;
} m610;

static SFORMAT StateRegs[] = {
	{ &m610.reg, 1, "EXPR" },
	{ 0 }
 };

static void Sync() {
	if (m610.reg & 0x10) { /* NROM-128 mode */
		setprg16(0x8000, m610.reg << 1);
		setprg16(0xC000, m610.reg << 1);
	} else { /* NROM-256 mode */
		setprg32(0x8000, m610.reg);
	}
	setchr8((m610.reg << 2) | (latch.data & 0x03));
	setmirror(((m610.reg >> 3) & 1) ^ 1);
}

static DECLFW(WriteReg) {
	m610.reg = V;
	Sync();
}

static void Reset(void) {
	m610.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	memset(&m610, 0, sizeof(m610));
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper610_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
