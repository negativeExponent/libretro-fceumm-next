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
	uint8_t reg[3];
} m234;

static SFORMAT StateRegs[] = {
	{ m234.reg, 3, "REGS" },
	{ 0 }
};

static void Sync(void) {
	if (m234.reg[0] & 0x40) {
		setprg32(0x8000, (m234.reg[0] & 0x0E) | (m234.reg[1] & 0x01));
		setchr8(((m234.reg[0] & 0x0E) << 2) | ((m234.reg[1] >> 4) & 0x07));
	} else {
		setprg32(0x8000, m234.reg[0] & 0x0F);
		setchr8(((m234.reg[0] & 0x0F) << 2) | ((m234.reg[1] >> 4) & 0x03));
	}
	setmirror((m234.reg[0] >> 7) ^ 0x01);
}

static DECLFR(ReadReg) {
	uint8_t ret = CartBR(A);

	switch (A & 0xFFF8) {
	case 0xFF80:
	case 0xFF88:
	case 0xFF90:
	case 0xFF98:
		if (!m234.reg[0]) {
			m234.reg[0] = ret;
			Sync();
		}
		break;
	case 0xFFC0:
	case 0xFFC8:
	case 0xFFD0:
	case 0xFFD8:
		if (!m234.reg[0]) {
			m234.reg[2] = ret;
			Sync();
		}
		break;
	case 0xFFE8:
	case 0xFFF0:
		m234.reg[1] = ret;
		Sync();
		break;
	}

	return ret;
}

static void Reset(void) {
	memset(&m234, 0, sizeof(m234));
	Sync();
}

static void Power(void) {
	memset(&m234, 0, sizeof(m234));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetReadHandler(0xFF80, 0xFFFF, ReadReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper234_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
