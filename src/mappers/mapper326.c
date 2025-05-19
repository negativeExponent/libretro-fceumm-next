/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022-2024 negativeExponent
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
 * NES 2.0 Mapper 326 is used for a bootleg version of Contra/Gryzor.
 * as implemented from
 * http://forums.nesdev.org/viewtopic.php?f=9&t=17352&p=218722#p218722
 */

#include "mapinc.h"

static uint8 prg[4], chr[16], nt[4];

static SFORMAT StateRegs[] = {
	{ prg, 4, "PREG" },
	{ chr, 16, "CREG" },
	{ 0 }
};

static void Sync(void) {
	int i;

	for (i = 0; i < 4; i++) {
		setprg8(0x8000 + (i << 13), prg[i]);
	}
	for (i = 0; i < 8; i++) {
		setchr1(i << 10, chr[i]);
	}
	for (i = 0; i < 3; i++) {
		setntamem(NTARAM + 0x400 * (chr[8 | i] & 0x01), 1, i);
	}
}

static DECLFW(M326Write) {
	switch (A & 0xE010) {
	case 0x8000:
	case 0xA000:
	case 0xC000:
		prg[A >> 13 & 3] = V;
		Sync();
		break;
	}

	if ((A & 0x8010) == 0x8010){
		chr[A & 0x0F] = V;
		Sync();
	}
}

static void M326Power(void) {
	int i;

	for (i = 0; i < 4; i++) {
		prg[i] = 0xFC | i;
	}
	for (i = 0; i < 8; i++) {
		chr[i] = i;
	}
	for (i = 0; i < 4; i++) {
		chr[8 | i] = (i >> 1) & 0x01;
	}

	Sync();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, M326Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper326_Init(CartInfo *info) {
	info->Power = M326Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
