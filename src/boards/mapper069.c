/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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

#include "fme7sound.h"

static uint8 cmdreg, preg[4], creg[8], mirr;
static uint8 IRQa;
static int32 IRQCount;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ &cmdreg, 1, "CMDR" },
	{ preg, 4, "PREG" },
	{ creg, 8, "CREG" },
	{ &mirr, 1, "MIRR" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQCount, 4, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	uint8 i;
	if ((preg[3] & 0xC0) == 0xC0)
		setprg8r(0x10, 0x6000, preg[3] & 0x3F);
	else
		setprg8(0x6000, preg[3] & 0x3F);
	setprg8(0x8000, preg[0]);
	setprg8(0xA000, preg[1]);
	setprg8(0xC000, preg[2]);
	setprg8(0xE000, ~0);
	for (i = 0; i < 8; i++)
		setchr1(i << 10, creg[i]);
	switch (mirr & 3) {
		case 0: setmirror(MI_V); break;
		case 1: setmirror(MI_H); break;
		case 2: setmirror(MI_0); break;
		case 3: setmirror(MI_1); break;
	}
}

static DECLFW(M069WRAMWrite) {
	if ((preg[3] & 0xC0) == 0xC0)
		CartBW(A, V);
}

static DECLFR(M069WRAMRead) {
	if ((preg[3] & 0xC0) == 0x40)
		return X.DB;
	else
		return CartBR(A);
}

static DECLFW(M069WriteIndex) {
	cmdreg = V & 0xF;
}

static DECLFW(M069WriteCMD) {
	switch (cmdreg) {
		case 0x0:
			creg[0] = V;
			Sync();
			break;
		case 0x1:
			creg[1] = V;
			Sync();
			break;
		case 0x2:
			creg[2] = V;
			Sync();
			break;
		case 0x3:
			creg[3] = V;
			Sync();
			break;
		case 0x4:
			creg[4] = V;
			Sync();
			break;
		case 0x5:
			creg[5] = V;
			Sync();
			break;
		case 0x6:
			creg[6] = V;
			Sync();
			break;
		case 0x7:
			creg[7] = V;
			Sync();
			break;
		case 0x8:
			preg[3] = V;
			Sync();
			break;
		case 0x9:
			preg[0] = V;
			Sync();
			break;
		case 0xA:
			preg[1] = V;
			Sync();
			break;
		case 0xB:
			preg[2] = V;
			Sync();
			break;
		case 0xC:
			mirr = V & 3;
			Sync();
			break;
		case 0xD:
			IRQa = V;
			X6502_IRQEnd(FCEU_IQEXT);
			break;
		case 0xE:
			IRQCount &= 0xFF00;
			IRQCount |= V;
			break;
		case 0xF:
			IRQCount &= 0x00FF;
			IRQCount |= V << 8;
			break;
	}
}

static void M069Power(void) {
	cmdreg = 0;
	IRQCount = 0xFFFF;
	IRQa = 0;
	Sync();
	SetReadHandler(0x6000, 0x7FFF, M069WRAMRead);
	SetWriteHandler(0x6000, 0x7FFF, M069WRAMWrite);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, M069WriteIndex);
	SetWriteHandler(0xA000, 0xBFFF, M069WriteCMD);
	SetWriteHandler(0xC000, 0xDFFF, FME7Sound_WriteIndex);
	SetWriteHandler(0xE000, 0xFFFF, FME7Sound_WriteCMD);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M069Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void M069IRQHook(int a) {
	if (IRQa) {
		IRQCount -= a;
		if (IRQCount <= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			IRQa = 0;
			IRQCount = 0xFFFF;
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper069_Init(CartInfo *info) {
	info->Power = M069Power;
	info->Close = M069Close;
	MapIRQHook = M069IRQHook;
	WRAMSIZE = 8192;
	WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
	GameStateRestore = StateRestore;
	FME7Sound_ESI();
	AddExState(&StateRegs, ~0, 0, 0);
}

void NSFAY_Init(void) {
	SetWriteHandler(0xC000, 0xDFFF, FME7Sound_WriteIndex);
	SetWriteHandler(0xE000, 0xFFFF, FME7Sound_WriteCMD);
	FME7Sound_ESI();
}
