/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 * Family Study Box by Fukutake Shoten
 */

#include "mapinc.h"

static uint8_t SWRAM[3072];
static uint8_t reg[4];

static SFORMAT StateRegs[] = {
	{ reg, 4, "DREG" },
	{ SWRAM, 3072, "SWRM" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, reg[0] >> 6);
	setprg16(0x8000, reg[1]);
	setprg16(0xc000, 0);
	setchr8(0);
}

static DECLFW(WriteReg) {
	if (A & 0x4203)
		reg[A & 3] = V;
	Sync();
}

static DECLFR(Read4) {
	switch (A) {
	case 0x4200:
		return 0x00;
	case 0x4201:
		return 0x00;
	case 0x4202:
		return 0x40;
	case 0x4203:
		return 0x00;
	}
	return 0xFF;
}

static DECLFR(ASWRAM) {
	return (SWRAM[A - 0x4400]);
}
static DECLFW(BSWRAM) {
	SWRAM[A - 0x4400] = V;
}

static void Power(void) {
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0xFFFF, CartBW);
	SetReadHandler(0x4200, 0x43FF, Read4);
	SetWriteHandler(0x4200, 0x43FF, WriteReg);
	SetReadHandler(0x4400, 0x4FFF, ASWRAM);
	SetWriteHandler(0x4400, 0x4FFF, BSWRAM);
	FCEU_CheatAddRAM(32, 0x6000, WRAM);
	reg[0] = reg[1] = reg[2] = reg[3];
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper186_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	WRAM = (uint8_t *)FCEU_gmalloc(32768);
	SetupCartPRGMapping(0x10, WRAM, 32768, 1);
	AddExState(WRAM, 32768, 0, "WRAM");
	AddExState(StateRegs, ~0, 0, NULL);
}
