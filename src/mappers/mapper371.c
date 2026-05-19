/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
 *  Copyright (C) 2025-2026 negativeExponent
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
} m371;

static SFORMAT StateRegs[] = {
	{ m371.reg, 8, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	uint32_t bank = ((m371.reg[1] << 4) & 0x10) | (m371.reg[0] & 0x0F);

	if ((m371.reg[0] & 0x70) == 0x50) {
		setprg16(0x8000, 4 + bank);
		setprg16(0x8000, 4 + bank);
	} else {
		setprg16(0x8000, bank & 0x03);
		setprg16(0xc000, 0x03);
	}
}

static void SyncMirror(void) {
	setmirror((m371.reg[1] >> 1) & 1);
}

static DECLFW(WriteReg) {
	uint8_t index = (A & 0x700) >> 8;

	m371.reg[index] = V;
	switch (index) {
	case 0:
		PEC586Hack = (m371.reg[0] & 0x80) ? TRUE : FALSE;
		SyncPRG();
		break;
	case 1:
		SyncPRG();
		SyncMirror();
		break;
	}
}

static void Power(void) {
	memset(m371.reg, 0, sizeof(m371.reg));
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	SyncPRG();
	SyncMirror();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5fff, WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncMirror();
}

void Mapper371_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
