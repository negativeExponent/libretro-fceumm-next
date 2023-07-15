/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 * NES 2.0 Mapper 125 - UNL-M125
 * FDS Conversion - Monty no Doki Doki Daisassō, Monty on the Run, cartridge code M125
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static uint8 reg;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ &reg, 1, "REG" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x6000, reg);
	setprg8(0x8000, ~3);
	setprg8(0xa000, ~2);
	setprg8r(0x10, 0xc000, 0);
	setprg8(0xe000, ~0);
	setchr8(0);
}

static DECLFW(M125Write) {
	reg = V;
	Sync();
}

static void M125Power(void) {
	FDSSound_Power();
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xC000, 0xDFFF, CartBW);
	SetWriteHandler(0x6000, 0x6000, M125Write);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M125Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper125_Init(CartInfo *info) {
	info->Power = M125Power;
	info->Close = M125Close;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
