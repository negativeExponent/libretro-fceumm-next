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

static struct {
	uint8_t reg[8];
	uint8_t bigbank;
	uint8_t reserve[3];
} m257;

static SFORMAT StateRegs[] = {
	{ m257.reg, 8, "REG" },
	{ &m257.bigbank, 4, "BIGB" },
	{ 0 }
};

static uint8_t bs_tbl[128] = {
	0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, /* 00 */
	0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, /* 10 */
	0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, /* 20 */
	0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, /* 30 */
	0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, /* 40 */
	0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, /* 50 */
	0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x00, 0x10, 0x20, 0x30, /* 60 */
	0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, /* 70 */
};

static uint8_t br_tbl[16] = {
	0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static void Sync(void) {
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	if (m257.bigbank) {
		setprg32(0x8000, m257.reg[0] & 0x07);
		if (!(m257.reg[0] & 0x010) && (m257.reg[0] & 0x40)) {
			setprg8(0x8000, (m257.reg[0] & 0x0F) | 0x20 | ((m257.reg[0] & 0x20) >> 1));
		}
		if ((m257.reg[0] & 0x18) == 0x18) {
			setmirror(MI_H);
		} else {
			setmirror(MI_V);
		}
	} else {
		setprg16(0x8000, bs_tbl[m257.reg[0] & 0x7F] >> 4);
		setprg16(0xc000, bs_tbl[m257.reg[0] & 0x7F] & 0xf);
		setmirror(MI_V);
	}
}

static DECLFW(Write5) {
	m257.reg[(A & 0x700) >> 8] = V;
	PEC586Hack = (m257.reg[0] & 0x80) ? TRUE : FALSE;
	/*	FCEU_printf("bs %04x %02x\n", A, V); */
	Sync();
}

static DECLFR(Read5) {
	/*	FCEU_printf("read %04x\n", A); */
	return (cpu.openbus & 0xD8) | br_tbl[m257.reg[4] >> 4];
}

static DECLFR(ReadCart) {
	if ((m257.reg[0] & 0x10) || ((m257.reg[0] & 0x40) && (A < 0xA000))) {
		return CartBR(A);
	}
	return PRGptr[0][((0x0107 | ((A >> 7) & 0x0F8)) << 10) | (A & 0x3FF)];
}

static void Power(void) {
	memset(&m257, 0, sizeof(m257));
	m257.bigbank = (PRGsize[0] == SIZE_512K) ? TRUE : FALSE;
	m257.reg[0] = m257.bigbank ? 0 : 0x0E;
	Sync();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, m257.bigbank ? ReadCart : CartBR);
	SetWriteHandler(0x5000, 0x5fff, Write5);
	SetReadHandler(0x5000, 0x5fff, Read5);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper257_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
