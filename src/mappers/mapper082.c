/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 *
 * Taito X1-017 board, battery backed
 * NES 2.0 Mapper 552 represents the actual way the mask ROM is connected and is thus
 * the correct bank order, while iNES Mapper 082 represents the bank order as it was
 * understood before January 2020 when the mapper was reverse-engineered.
 */

#include "mapinc.h"

static uint8 creg[6], preg[3], prot[3], ctrl;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;
static int mappernum;

static SFORMAT StateRegs[] =
{
	{ preg, 3, "PREGS" },
	{ creg, 6, "CREGS" },
	{ prot, 3, "PROT" },
	{ &ctrl, 1, "CTRL" },

	{ 0 }
};

static uint32 getPRGBank(uint8 V) {
	if (mappernum == 552) {
		return (((V << 5) & 0x20) |
		((V << 3) & 0x10) |
		((V << 1) & 0x08) |
		((V >> 1) & 0x04) |
		((V >> 3) & 0x02) |
		((V >> 5) & 0x01));
	}
	return V >> 2;
}

static void Sync(void) {
	uint32 swap = ((ctrl & 2) << 11);
	setchr2(0x0000 ^ swap, creg[0] >> 1);
	setchr2(0x0800 ^ swap, creg[1] >> 1);
	setchr1(0x1000 ^ swap, creg[2]);
	setchr1(0x1400 ^ swap, creg[3]);
	setchr1(0x1800 ^ swap, creg[4]);
	setchr1(0x1C00 ^ swap, creg[5]);
	setprg8r(0x10, 0x6000, 0);
	setprg8(0x8000, getPRGBank(preg[0]));
	setprg8(0xA000, getPRGBank(preg[1]));
	setprg8(0xC000, getPRGBank(preg[2]));
	setprg8(0xE000, ~0);
	setmirror(ctrl & 1);
}

static DECLFR(ReadWRAM) {
	if (((A >= 0x6000) && (A <= 0x67FF) && (prot[0] == 0xCA)) ||
	    ((A >= 0x6800) && (A <= 0x6FFF) && (prot[1] == 0x69)) ||
	    ((A >= 0x7000) && (A <= 0x73FF) && (prot[2] == 0x84))) {
			return CartBR(A);
	}
	return X.DB;
}

static DECLFW(WriteWRAM) {
	if (((A >= 0x6000) && (A <= 0x67FF) && (prot[0] == 0xCA)) ||
	    ((A >= 0x6800) && (A <= 0x6FFF) && (prot[1] == 0x69)) ||
	    ((A >= 0x7000) && (A <= 0x73FF) && (prot[2] == 0x84))) {
			CartBW(A, V);
	}
}

static DECLFW(M082Write) {
	switch (A & 0x0F) {
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
	case 0x4:
	case 0x5: creg[A & 7] = V; break;
	case 0x6: ctrl = V & 3; break;
	case 0x7: prot[0] = V; break;
	case 0x8: prot[1] = V; break;
	case 0x9: prot[2] = V; break;
	case 0xA: preg[0] = V; break;
	case 0xB: preg[1] = V; break;
	case 0xC: preg[2] = V; break;
	}
	Sync();
}

static void M082Power(void) {
	Sync();
	SetReadHandler(0x6000, 0xffff, CartBR);
	SetReadHandler(0x6000, 0x73ff, ReadWRAM);
	SetWriteHandler(0x6000, 0x73ff, WriteWRAM);
	SetWriteHandler(0x7ef0, 0x7eff, M082Write); /* external WRAM might end at $73FF */
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M082Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper082_Init(CartInfo *info) {
	mappernum = info->mapper;
	info->Power = M082Power;
	info->Close = M082Close;

	WRAMSIZE = 8192;
	WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}

void Mapper552_Init(CartInfo *info) {
	Mapper082_Init(info);
}
