/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2011 CaH4e3
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
 * FDS Conversion - Metroid - Jin Ji Zhi Ling (Kaiser)(KS7037)[U][!]
 * NES 2.0 Mapper 307 - UNL-KS7037
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static uint8 reg[8], cmd;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ &cmd, 1, "CMD" },
	{ reg, 8, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg4r(0x10, 0x6000, 0);
	setprg4(0x7000, 15);
	setprg8(0x8000, reg[6]);
	setprg4(0xA000, ~3);
	setprg4r(0x10, 0xB000, 1);
	setprg8(0xC000, reg[7]);
	setprg8(0xE000, ~0);
	setchr8(0);
	setmirrorw(reg[2] & 1, reg[4] & 1, reg[3] & 1, reg[5] & 1);
}

static DECLFW(M307Write) {
	switch (A & 0xE001) {
	case 0x8000: cmd = V & 7; break;
	case 0x8001: reg[cmd] = V; Sync(); break;
	}
}

static void M307Power(void) {
	FDSSoundPower();
	reg[0] = reg[1] = reg[2] = reg[3] = reg[4] = reg[5] = reg[6] = reg[7] = 0;
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0x9FFF, M307Write);
	SetWriteHandler(0xA000, 0xBFFF, CartBW);
	SetWriteHandler(0xC000, 0xFFFF, M307Write);
}

static void Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper307_Init(CartInfo *info) {
	info->Power = M307Power;
	info->Close = Close;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
