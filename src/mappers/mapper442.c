/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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

/* TODO: No keyboard input (Golden Key) */

#include "mapinc.h"

static uint8 reg[8];

static SFORMAT StateRegs[] = {
	{ reg, 8, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, ((reg[0] >> 1) & 0x20) | (reg[0] & 0x1F));
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
}

static DECLFW(writeReg) {
	reg[(A >> 8) & 0x07] = V;
	PEC586Hack = (reg[0] & 0x80) ? TRUE : FALSE;
	Sync();
}

static void M442Reset(void) {
	memset(reg, 0, sizeof(reg));
	Sync();
}

static void M442Power(void) {
	memset(reg, 0, sizeof(reg));
	Sync();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, writeReg);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper442_Init(CartInfo *info) {
	info->Power = M442Power;
	info->Reset = M442Reset;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);

	AddExState(StateRegs, ~0, 0, "NULL");
}
