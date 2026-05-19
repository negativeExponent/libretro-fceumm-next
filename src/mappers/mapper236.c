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
	uint8_t reg[2];
} m236;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m236.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t prg;
	uint8_t chr;

	if (ROM.chr.size) {
		prg = m236.reg[1] & 0x0F;
		chr = m236.reg[0] & 0x0F;
	} else {
		prg = (m236.reg[1] & 0x07) | (m236.reg[0] << 3);
		chr = 0;
	}
	switch (m236.reg[1] >> 4 & 3) {
	case 0:
	case 1:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 0x07);
		break;
	case 2:
		setprg32(0x8000, prg >> 1);
		break;
	case 3:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
		break;
	}
	setchr8(chr);
	setmirror(((m236.reg[0] >> 5) & 0x01) ^ 0x01);
}

static DECLFR(ReadDIP) {
	if (((m236.reg[1] >> 4) & 0x03) == 1) {
		return CartBR((A & 0xFFF0) | (dipsw & 0x0F));
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m236.reg[(A >> 14) & 0x01] = A & 0xFF;
	Sync();
}

static void Power(void) {
	memset(&m236, 0, sizeof(m236));
	dipsw = 0;
	Sync();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
}

static void Reset(void) {
	memset(&m236, 0, sizeof(m236));
	++dipsw;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper236_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
