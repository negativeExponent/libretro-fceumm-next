/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
	uint8_t reg[4];
} m466;

static SFORMAT StateRegs[] = {
	{ m466.reg, 4, "EXPR" },
	{ 0 }
};

static void Sync(void) {
	uint16_t prg = (m466.reg[1] << 5) | ((m466.reg[0] << 1) & 0x1E) | ((m466.reg[0] >> 5) & 0x01);

	/* Return open bus when selecting unpopulated PRG chip */
	if ((prg & 0x20) && (PRGsize[0] < (1024 * 1024))) {
		unsetcpu32(0x8000);
	} else {
		if (m466.reg[0] & 0x40) {
			if (m466.reg[0] & 0x10) {
				setprg16(0x8000, prg);
				setprg16(0xC000, prg);
			} else {
				setprg32(0x8000, prg >> 1);
			}
		} else {
			setprg16(0x8000, (prg & ~0x07) | (m466.reg[2] & 0x07));
			setprg16(0xC000, (prg & ~0x07) | 0x07);
		}
	}
	setprg8r(0x10, 0x6000, 0);
	setchr8(0);
	setmirror(((m466.reg[0] >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	m466.reg[(A >> 11) & 0x01] = A & 0xFF;
	Sync();
}

static DECLFW(WriteLatch) {
	m466.reg[2] = V;
	Sync();
}

static void Reset(void) {
	memset(&m466, 0, sizeof(m466));
	Sync();
}

static void Power(void) {
	memset(&m466, 0, sizeof(m466));
	Sync();

	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper466_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	AddExState(StateRegs, ~0, 0, NULL);
}
