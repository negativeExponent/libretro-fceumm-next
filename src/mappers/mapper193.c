/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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
 *
 * MEGA-SOFT WAR IN THE GULF
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg[8];
	uint8_t mirror;
} m193;

static SFORMAT StateRegs[] = {
	{ &m193.mirror, 1, "MIRR" },
	{ m193.reg, 8, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m193.reg[3]);
	setprg8(0xA000, ~0x02);
	setprg8(0xC000, ~0x01);
	setprg8(0xE000, ~0x00);
}

static void SyncCHR(void) {
	setchr4(0x0000, m193.reg[0] >> 2);
	setchr2(0x1000, m193.reg[1] >> 1);
	setchr2(0x1800, m193.reg[2] >> 1);
}

static void SyncMirror(void) {
	setmirror((m193.mirror & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	m193.reg[A & 0x07] = V;
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m193, 0, sizeof(m193));

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, CartBW);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper193_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
