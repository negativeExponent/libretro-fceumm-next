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
} m041;

static SFORMAT StateRegs[] = {
	{ m041.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, m041.reg[0] & 0x07);
	setchr8(((m041.reg[0] >> 1) & 0x0C) | (m041.reg[1] & 0x03));
	setmirror(((m041.reg[0] >> 5) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg0) {
	m041.reg[0] = A;
	Sync();
}

static DECLFW(WriteReg1) {
	if (m041.reg[0] & 0x04) {
		/* bus conflict */
		m041.reg[1] = (V & CartBR(A));
		Sync();
	}
}

static void Reset(void) {
	m041.reg[0] = m041.reg[1] = 0;
	Sync();
}

static void Power(void) {
	m041.reg[0] = m041.reg[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x67FF, WriteReg0);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg1);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper041_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
