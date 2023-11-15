/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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

static uint8 reg[8];
static uint32 lastnt = 0;

static SFORMAT StateRegs[] =
{
	{ reg, 8, "REG" },
	{ &lastnt, 4, "LNT" },
	{ 0 }
};

static void Sync(void) {
    uint32 prg =((reg[1] << 4) & 0x10) | (reg[0] & 0x0F);
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	if((reg[0] & 0x70) == 0x50) {
		setprg16(0x8000, 4 + prg);
        setprg16(0x8000, 4 + prg);
	} else {
		setprg16(0x8000, prg & 0x03);
		setprg16(0xc000, 0x03);
	}
    setmirror((reg[1] >> 1) & 1);
}

static DECLFW(M371Write) {
	reg[(A & 0x700) >> 8] = V;
	PEC586Hack = (reg[0] & 0x80) ? TRUE : FALSE;
/*	FCEU_printf("bs %04x %02x\n", A, V); */
	Sync();
}

static void M371Power(void) {
    memset(reg, 0, sizeof(reg));
	Sync();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5fff, M371Write);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M371Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper371_Init(CartInfo *info) {
	info->Power = M371Power;
	info->Close = M371Close;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(StateRegs, ~0, 0, NULL);
}
