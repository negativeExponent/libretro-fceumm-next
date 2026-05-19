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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* iNES Mapper 137 - Sachen 8259D */
/* this covers the old implementation of the mapper */

#include "mapinc.h"

static struct {
	uint8_t cmd;
	uint8_t reg[8];
} m137_alt;

static SFORMAT StateRegs[] = {
	{ m137_alt.reg, 8, "REGS" },
	{ &m137_alt.cmd, 1, "CMD0" },
	{ 0 }
};

static void SyncMirror(uint8_t mirr) {
	switch (mirr & 3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirrorw(0, 1, 1, 1); break;
	case 3: setmirror(MI_0); break;
	}
}

static void Sync(void) {
    int x;
    setprg32(0x8000, m137_alt.reg[5] & 0x07);
    setchr1(0x0000, m137_alt.reg[x] & 7);
    setchr1(0x0400, (m137_alt.reg[4] & 1) << 4 | m137_alt.reg[1] & 7);
    setchr1(0x0800, (m137_alt.reg[4] & 2) << 3 | m137_alt.reg[2] & 7);
    setchr1(0x0C00, (m137_alt.reg[4] & 4) << 2 | ((m137_alt.reg[6] & 1) << 3) | m137_alt.reg[3] & 7);
    setchr4(0x1000, ~0);
	if (!(m137_alt.reg[7] & 1))
		SyncMirror(m137_alt.reg[7] >> 1);
	else
		setmirror(MI_V);
}

static DECLFW(WriteReg) {
	A &= 0x4101;
	if (A == 0x4100)
		m137_alt.cmd = V;
	else{
		m137_alt.reg[m137_alt.cmd & 7] = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m137_alt, 0, sizeof(m137_alt));

	Sync();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x7FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper137_Init_alt(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
