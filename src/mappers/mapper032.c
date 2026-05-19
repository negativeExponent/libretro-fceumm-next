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
	uint8_t prg[2], chr[8], mode;
} m032;

static SFORMAT StateRegs[] = {
	{ m032.prg, 4, "PREG" },
	{ m032.chr, 8, "CREG" },
	{ &m032.mode, 1, "MODE" },
	{ 0 }
};

static void SyncPRG(void) {
	uint16_t pswap = (m032.mode & 0x02) ? 0x4000 : 0;

	setprg8(0x8000 ^ pswap, m032.prg[0]);
	setprg8(0xA000, m032.prg[1]);
	setprg8(0xC000 ^ pswap, 0xFE);
	setprg8(0xE000, 0xFF);
}

static void SyncCHR(void) {
	setchr1(0x0000, m032.chr[0]);
	setchr1(0x0400, m032.chr[1]);
	setchr1(0x0800, m032.chr[2]);
	setchr1(0x0C00, m032.chr[3]);
	setchr1(0x1000, m032.chr[4]);
	setchr1(0x1400, m032.chr[5]);
	setchr1(0x1800, m032.chr[6]);
	setchr1(0x1C00, m032.chr[7]);
}

static void SyncMirror(void) {
	if (iNESCart.submapper == 1) {
		setmirror(MI_1);
	} else {
		setmirror((m032.mode & 0x01) ^ 0x01);
	}
}

static DECLFW(WritePRG) {
	m032.prg[(A >> 13) & 0x01] = V;
	SyncPRG();
}

static DECLFW(WriteMode) {
	m032.mode = V;
	if (iNESCart.submapper == 1) {
		m032.mode &= ~0x02;
	}
	SyncPRG();
	SyncMirror();
}

static DECLFW(WriteCHR) {
	m032.chr[A & 0x07] = V;
	SyncCHR();
}

static void Power(void) {
	m032.prg[0] = 0;
	m032.prg[1] = 1;
	m032.chr[0] = 0;
	m032.chr[1] = 1;
	m032.chr[2] = 2;
	m032.chr[3] = 3;
	m032.chr[4] = 4;
	m032.chr[5] = 5;
	m032.chr[6] = 6;
	m032.chr[7] = 7;
	m032.mode = 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, WriteMode);
	SetWriteHandler(0xA000, 0xAFFF, WritePRG);
	SetWriteHandler(0xB000, 0xBFFF, WriteCHR);

	if (WRAM) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper032_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	}
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
}
