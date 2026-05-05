/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m609;

static SFORMAT StateRegs[] = {
	{ &m609.reg, 4, "EXPR" },
	{ 0 }
 };

static void Sync(void) {
	uint16_t bank;

	for (bank = 0x8000; bank <= 0xF000; bank += 0x2000) {
		uint16_t val = m609.reg;
		if (m609.reg & 0x20) val = val & ~0x01 | bank >>13 &1;
		if (m609.reg & 0x40) val = val & ~0x02 | bank >>13 &2;
		setprg8(bank, val);
	}
	setchr8(0);
	setmirror(((m609.reg >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m609.reg = V;
		Sync();
	}
}

static void Reset(void) {
	m609.reg = 0;
	Sync();
}

static void Power(void) {
	memset(&m609, 0, sizeof(m609));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0x5FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper609_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
