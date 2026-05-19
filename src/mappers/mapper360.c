/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 360 - Bit Corp's 31-in-1 multicart (3150) */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m360;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m360.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (iNESCart.submapper == 0) {
		m360.reg = (0x20 | dipsw);
	}
	if (!(m360.reg & 0x20)) {
		setprg8(0x8000, 0x40);
		setprg8(0xA000, 0x40);
		setprg8(0xC000, 0x40);
		setprg8(0xE000, 0x40);
	} else {
		uint8_t bank = m360.reg & 0x1F;
		/* dip 0 and 1 is the same game SMB) */
		if (bank < 2) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
	setchr8(m360.reg);
	setmirror(((m360.reg & 0x10) >> 4) ^ 1);
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m360.reg = V;
		Sync();
	}
}

static void Reset(void) {
	memset(&m360, 0, sizeof(m360));
	if (iNESCart.submapper == 0) {
		dipsw = (dipsw + 1) & 31;
	} else {
		dipsw = 0;
	}
	Sync();
	FCEU_printf("dipsw = %d\n", dipsw);
}

static void Power(void) {
	memset(&m360, 0, sizeof(m360));
	dipsw = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0XFFFF, CartBW);
	if (iNESCart.submapper == 1) {
		SetWriteHandler(0x4100, 0x4FFF, WriteReg);
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper360_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
