/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 * NES 2.0 Mapper 346 - Kaiser 7012
 * UNL-KS7012
 * FDS Conversion
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m346;

static SFORMAT StateRegs[] = {
	{ &m346.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg32(0x8000, m346.reg);
	setchr8(0);
}

static DECLFW(WriteReg) {
	/*	FCEU_printf("bs %04x %02x\n",A,V); */
	switch (A) {
	case 0xE0A0:
		m346.reg = 0;
		Sync();
		break;
	case 0xEE36:
		m346.reg = 1;
		Sync();
		break;
	}
}

static void Power(void) {
	m346.reg = 1;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xE000, 0xEFFF, WriteReg);

	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Reset(void) {
	m346.reg = 1;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

static void Close(void) {
}

void Mapper346_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
