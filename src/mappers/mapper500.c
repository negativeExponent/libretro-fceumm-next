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

/* NES 2.0 Mapper 500 */
/* 15-in-1 (YHC000) (Unl) */
/* UNIF: BMC-Yhc-Unrom-Cart */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m500;

static SFORMAT StateRegs[] = {
	{ m500.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg4(0x7000, 0);
	setprg16(0x8000, (m500.reg[0] << 3) | (latch.data & 0x07));
	setprg16(0xC000, (m500.reg[0] << 3) | 0x07);
	setchr8(0);
	setmirror(m500.reg[1] & 0x01);
}

static DECLFW(WriteReg) {
	if (!(m500.reg[1] & 0x80)) {
		m500.reg[A & 0x01] = V;
		Sync();
	}
}

static void Reset(void) {
	memset(&m500, 0, sizeof(m500));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m500, 0, sizeof(m500));
	Latch_Power();
	SetReadHandler(0x7000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x6FFF, WriteReg);
}

void Mapper500_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
