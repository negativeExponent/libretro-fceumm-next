/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

/* Mapper 150 - SA-015 / SA-630 / Unif UNL-Sachen-74LS374N */
/* Mapper 243 - SA-020A */

#include "mapinc.h"

static uint8 dip;
static uint8 cmd;
static uint8 reg[8];

static void Sync(void) {
	uint8 chrBank;

	if (iNESCart.mapper == 243) {
        chrBank = (reg[2] & 0x01) | ((reg[4] << 1) & 0x02) | (reg[6] << 2);
    } else {
        chrBank = (reg[6] & 0x03) | ((reg[4] << 2) & 0x04) | (reg[2] << 3);
    }

	setprg32(0x8000, (reg[2] & 0x01) | reg[5]);
	setchr8(chrBank);

	switch ((reg[7] >> 1) & 0x03) {
	case 0: setmirrorw(0, 1, 1, 1); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_V); break;
	case 3: setmirror(MI_0); break;
	}
}

static DECLFR(M150Read) {
	if ((A & 0x101) == 0x101) {
		if (dip & 1)
			return (reg[cmd] & 0x03) | (X.DB & 0xFC);
		else
			return (reg[cmd] & 0x07) | (X.DB & 0xF8);
	}
	return X.DB;
}

static DECLFW(M150Write) {
	if (dip & 0x01)
		V |= 0x04;
	switch (A & 0x101) {
	case 0x100:
		cmd = V & 0x07;
		break;
	case 0x101:
		reg[cmd] = V & 0x07;
		Sync();
		break;
	}
}

static void M150Restore(int version) {
	Sync();
}

static void M150Reset(void) {
	dip ^= 0x01;
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	reg[4] = reg[5] = reg[6] = reg[7] = 0;
	Sync();
}

static void M150Power(void) {
	dip = 0;
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	reg[4] = reg[5] = reg[6] = reg[7] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
    SetReadHandler(0x4100, 0x5FFF, M150Read);
	SetWriteHandler(0x4100, 0x5FFF, M150Write);
}

void Mapper150_Init(CartInfo *info) {
	info->Power = M150Power;
	info->Reset = M150Reset;
	GameStateRestore = M150Restore;
	AddExState(reg, 8, 0, "REGS");
	AddExState(&cmd, 1, 0, "CMD");
}
