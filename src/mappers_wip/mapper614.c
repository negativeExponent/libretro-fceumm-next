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
	uint8_t reg[3];
} m614;

static SFORMAT StateRegs[] = {
	{ m614.reg, 3, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	uint16_t prg = (m614.reg[0] & 0x30) | ((m614.reg[1] << 3) & 0x08);

	if (m614.reg[0] & 0x08) {
		if (m614.reg[0] & 0x40) {
			setprg32(0x8000, ((prg >> 1) & 0x1C) | ((m614.reg[0] >> 1) & 0x03));
		} else {
			setprg16(0x8000, (prg & 0x38) | (m614.reg[2] & 0x07));
			setprg16(0xC000, (prg & 0x38) | (m614.reg[2] & 0x07));
		}
	} else {
		setprg16(0x8000, (prg & 0x38) | (m614.reg[2] & 0x07));
		setprg16(0xC000, (prg & 0x38) | 0x07);
	}
	setchr8(0);
	setmirror(((m614.reg[0] >> 7) & 1) ^ 1);
}

static DECLFW(WriteReg) {
	m614.reg[(A >> 13) & 0x02] = V;
	if (~A & 0x4000) {
		m614.reg[1] = A & 0xFF;
	}
	Sync();
}

static void Reset(void) {
	m614.reg[0] = 0;
	m614.reg[1] = 0;
	m614.reg[2] = 0;
	Sync();
}

static void Power(void) {
	m614.reg[0] = 0;
	m614.reg[1] = 0;
	m614.reg[2] = 0;
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper614_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
