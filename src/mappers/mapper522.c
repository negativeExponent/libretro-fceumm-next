/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2011 CaH4e3
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * NES 2.0 Mapper 522 is used for Whirlwind Manu's cartridge conversion of the
 * Famicom Disk System game 風雲 少林拳 (Fūun Shōrinken).
 * Its UNIF board name is UNL-LH10.
 */

#include "mapinc.h"

static struct {
	uint8_t reg[8], cmd;
} m522;

static SFORMAT StateRegs[] = {
	{ &m522.cmd, 1, "CMD" },
	{ m522.reg, 8, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m522.reg[6]);
	setprg8(0xA000, m522.reg[7]);
}

static DECLFW(WriteReg) {
	if (A & 0x0001) {
		m522.reg[m522.cmd & 0x07] = V;
		switch (m522.cmd & 0x07) {
		case 6:
		case 7:
			SyncPRG();
			break;
		default:
			break;
		}
	} else {
		m522.cmd = V;
	}
}

static void Power(void) {
	memset(&m522, 0, sizeof(m522));
	setprg8(0x6000, 0xFE);
	setprg8r(0x10, 0xC000, 0);
	setprg8(0xE000, 0xFF);
	setchr8(0);
	setmirror(iNESCart.mirror);
	SyncPRG();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, WriteReg);
	SetWriteHandler(0xC000, 0xDFFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
}

void Mapper522_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
