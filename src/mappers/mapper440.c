/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024-2025-2026 negativeExponent
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

static const uint8_t addr_order_lut[15] = {
    11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 12, 13, 14
};

static const uint8_t data_order_lut[15] = {
	 7, 6, 5, 4, 3, 2, 1, 0
};

static const uint8_t protect_lut[16] = {
	0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static struct {
	uint8_t reg[8];
	uint8_t extra;
} m440;

static SFORMAT StateRegs[] = {
	{ m440.reg, 8, "REG" },
	{ &m440.extra, 1, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	setprg32(0x8000, (m440.reg[0] >> 5) & 0x03);
	setmirror((m440.reg[1] >> 1) & 1);
}

static DECLFR(Read5000) {
	/*	FCEU_printf("read %04x\n", A); */
	switch (A & 0x700) {
	case 0x300:
		m440.extra ^= 4;
		return m440.extra;
	case 0x500:
		return (cpu.openbus & 0xD8) | protect_lut[m440.reg[4] >> 4];
	}
	return cpu.openbus;
}

static DECLFW(Write5000) {
	m440.reg[(A & 0x700) >> 8] = V;
	PEC586Hack = (m440.reg[0] & 0x80) ? TRUE : FALSE;
	/*	FCEU_printf("bs %04x %02x\n", A, V); */
	Sync();
}

static DECLFR(ReadPRG) {
	if (m440.reg[0] & 1) {
		uint16_t encAddr = A & 0x7FFF;
		uint16_t decAddr = 0;
		uint8_t encData = 0;
		uint8_t decData = 0;
		uint8_t bit;

		for (bit = 0; bit < 15; bit++) {
			decAddr |= (encAddr >> addr_order_lut[bit] & 0x01) << bit;
		}

		encData = CartBR(0x8000 | decAddr);
		for (bit = 0; bit < 8; bit++) {
			decData |= (encData >> data_order_lut[bit] & 0x01) << bit;
		}
		return decData;
	}
	return CartBR(A);
}

static void Power(void) {
	memset(m440.reg, 0, sizeof(m440.reg));
	m440.reg[0] = 0x0E;
	Sync();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, ReadPRG);
	SetReadHandler(0x5000, 0x5fff, Read5000);
	SetWriteHandler(0x5000, 0x5fff, Write5000);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}


static void StateRestore(int version) {
	Sync();
}

void Mapper440_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
