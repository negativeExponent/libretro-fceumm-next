/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
} m431;

static SFORMAT StateRegs[] = {
	{ m431.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = ((m431.reg[0] >> 2) & ~0x07) | (m431.reg[1] & 0x07);

	setprg16(0x8000, bank);
	setprg16(0xC000, bank | 0x07);
	setchr8(0);
	setmirror((m431.reg[0] & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	m431.reg[(A >> 14) & 0x01] = V;
	Sync();
}

static void Reset(void) {
	memset(&m431, 0, sizeof(m431));
	Sync();
}

static void Power(void) {
	memset(&m431, 0, sizeof(m431));
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper431_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
