/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022-2025-2026 negativeExponent
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

static struct {
	uint8_t prg[4], chr[8], nmt[4];
} m326;

static SFORMAT StateRegs[] = {
	{ m326.prg, 4, "PREG" },
	{ m326.chr, 8, "CREG" },
	{ m326.nmt, 4, "NREG" },
	{ 0 }
};

static void SyncPRG(void) {
	int i;

	for (i = 0; i < 4; i++) {
		setprg8(0x8000 + (i * 0x2000), m326.prg[i]);
	}
}

static void SyncCHR(void) {
	int i;

	for (i = 0; i < 8; i++) {
		setchr1(i * 0x400, m326.chr[i]);
	}
}

static void SyncNMT(void) {
	int i;

	for (i = 0; i < 4; i++) {
		setntamem(NTARAM + (m326.nmt[i] * 0x400), TRUE, i);
	}
}

static DECLFW(WriteReg) {
	switch (A & 0xE010) {
	case 0x8000:
	case 0xA000:
	case 0xC000:
		m326.prg[(A >> 13) & 0x03] = V;
		SyncPRG();
		break;
	case 0xE000:
		break;
	case 0x8010:
	case 0xA010:
	case 0xC010:
	case 0xE010:
		if (A & 0x08) {
			m326.nmt[A & 0x07] = V;
			SyncNMT();
		} else {
			m326.chr[A & 0x07] = V;
			SyncCHR();
		}
		break;
	}
}

static void Power(void) {
	int i;

	for (i = 0; i < 4; i++) {
		m326.prg[i] = 0xFC + i;
	}
	for (i = 0; i < 8; i++) {
		m326.chr[i] = i;
	}
	for (i = 0; i < 4; i++) {
		m326.nmt[i] = (i >> 1) & 0x01;
	}

	SyncPRG();
	SyncCHR();
	SyncNMT();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncNMT();
}

void Mapper326_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
