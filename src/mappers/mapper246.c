/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 */

#include "mapinc.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[4];
} m246;

static SFORMAT StateRegs[] = {
	{ m246.prg, 4, "PREG" },
	{ m246.chr, 4, "CREG" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m246.prg[0]);
	setprg8(0xA000, m246.prg[1]);
	setprg8(0xC000, m246.prg[2]);
	setprg8(0xE000, m246.prg[3]);
}

static void SyncCHR(void) {
	setchr2(0x0000, m246.chr[0]);
	setchr2(0x0800, m246.chr[1]);
	setchr2(0x1000, m246.chr[2]);
	setchr2(0x1800, m246.chr[3]);
}

static DECLFW(Write6) {
	switch (A & 0x07) {
	case 0:
	case 1:
	case 2:
	case 3:
		m246.prg[A & 0x03] = V;
		SyncPRG();
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		m246.chr[A & 0x03] = V;
		SyncCHR();
		break;
	}
}

static DECLFR(ReadF) {
	uint8_t ret = CartBR(A);

	if ((A & 0xFFE4) == 0xFFE4) {
		size_t prgOffset = (((m246.prg[3] | 0x10) << 13) | (A & 0x1FFF));
		ret = PRGptr[0][prgOffset & (ROM.prg.size - 1)];
	}
	return ret;
}

static void Power(void) {
	m246.prg[0] = 0;
	m246.prg[1] = 1;
	m246.prg[2] = 0xFE;
	m246.prg[3] = 0xFF;

	SyncPRG();
	SyncCHR();

	setprg2r(0x10, 0x6800, 0);

	SetReadHandler(0x6800, 0x6FFF, CartBR);
	SetReadHandler(0x8000, 0xEFFF, CartBR);
	SetReadHandler(0xF000, 0xFFFF, ReadF);

	SetWriteHandler(0x6000, 0x601F, Write6);
	SetWriteHandler(0x6800, 0x6FFF, CartBW);

	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
}

void Mapper246_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 2048;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}
