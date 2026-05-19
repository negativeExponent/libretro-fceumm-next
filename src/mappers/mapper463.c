/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
	uint8_t reg[4];
} m463;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m463.reg, 4, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	if (m463.reg[0] & 0x04) {
		setprg16(0x8000, m463.reg[1]);
		setprg16(0xC000, m463.reg[1]);
	} else {
		setprg32(0x8000, m463.reg[1] >> 1);
	}
	setchr8(m463.reg[2]);
	setmirror((m463.reg[0] & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	if (A & (0x10 << dipsw)) {
		m463.reg[A & 0x03] = V;
		Sync();
	}
}

static void Reset(void) {
	memset(&m463, 0, sizeof(m463));
	dipsw = (dipsw + 1) & 0x01;
	Sync();
}

static void Power(void) {
	memset(&m463, 0, sizeof(m463));
	dipsw = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper463_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
