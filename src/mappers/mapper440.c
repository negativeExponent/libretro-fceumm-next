/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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
static uint8 extra = 0;

static SFORMAT StateRegs[] =
{
	{ reg, 8, "REG" },
	{ &extra, 1, "EXPR" },
	{ 0 }
};

static uint8_t ba_tbl[15] = {
    11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 12, 13, 14
};

static uint8 bs_tbl[15] = {
	 7, 6, 5, 4, 3, 2, 1, 0
};

static uint8 br_tbl[16] = {
	0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static void Sync(void) {
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
    setprg32(0x8000, (reg[0] >> 5) & 3);
    setmirror((reg[1] >> 1) & 1);
}

static DECLFW(M440Write) {
	reg[(A & 0x700) >> 8] = V;
	PEC586Hack = (reg[0] & 0x80) ? TRUE : FALSE;
/*	FCEU_printf("bs %04x %02x\n", A, V); */
	Sync();
}

static DECLFR(M440Read) {
/*	FCEU_printf("read %04x\n", A); */
    switch (A & 0x700) {
    case 0x300: extra ^= 4; return extra;
    case 0x500: return (cpu.openbus & 0xD8) | br_tbl[reg[4] >> 4];
    }
    return cpu.openbus;
}

static DECLFR(M440ReadHi) {
	if (reg[0] & 1) {
		uint16 encAddr = A & 0x7000;
		uint16 decAddr = 0;
        uint8 encData = 0;
        uint8 decData = 0;
        uint8 bit;

		for (bit = 0; bit < 15; bit++) {
			decAddr |= (encAddr >> ba_tbl[bit] & 1) << bit;
        }

		encData = CartBR(0x8000 | decAddr);
		for (bit = 0; bit < 8; bit++) {
			decData |= (encData >> bs_tbl[bit] & 1) << bit;
        }
		return decData;
	}
	return CartBR(A);
}

static void M440Power(void) {
    memset(reg, 0, sizeof(reg));
    reg[0] = 0x0E;
    extra = 0;
	Sync();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, M440ReadHi);
	SetWriteHandler(0x5000, 0x5fff, M440Write);
	SetReadHandler(0x5000, 0x5fff, M440Read);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M440Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper440_Init(CartInfo *info) {
	info->Power = M440Power;
	info->Close = M440Close;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(StateRegs, ~0, 0, NULL);
}
