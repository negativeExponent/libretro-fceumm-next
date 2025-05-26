/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2025 negativeExponent
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

/* Mapper 053 - BMC-Supervision16in1 */

#include "mapinc.h"

static struct {
	uint8 reg[2];
} m053;

static SFORMAT StateRegs[] = {
	{ m053.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x6000, (((m053.reg[0] & 0x0F) << 4) | 0x0F));
	if (m053.reg[0] & 0x10) {
		setprg16(0x8000, (((m053.reg[0] & 0x0F) << 3) | (m053.reg[1] & 0x07)));
		setprg16(0xc000, (((m053.reg[0] & 0x0F) << 3) | 0x07));
	} else {
		setprg32r(1, 0x8000, 0);
	}
	setchr8(0);
	setmirror(((m053.reg[0] & 0x20) >> 5) ^ 0x01);
}

static DECLFW(WriteReg0) {
	if (!(m053.reg[0] & 0x10)) {
		m053.reg[0] = V;
		Sync();
	}
}

static DECLFW(WriteReg1) {
	m053.reg[1] = V;
	Sync();
}

static void Power(void) {
	SetWriteHandler(0x6000, 0x7FFF, WriteReg0);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg1);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	m053.reg[0] = m053.reg[1] = 0;
	Sync();
}

static void Reset(void) {
	m053.reg[0] = m053.reg[1] = 0;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper053_Init(CartInfo *info) {
	size_t ssize = ROM.prg.size - 32768;

	if (ssize == (2048 * 1024)) { /* Supervision 16-in-1 */
		uint8 *buffer = (uint8 *)FCEU_malloc(ssize);

		ROM.misc.size = 32768;
		ROM.misc.data = (uint8 *)FCEU_malloc(ROM.misc.size);

		memcpy(ROM.misc.data, ROM.prg.data, ROM.misc.size);
		memcpy(buffer, ROM.prg.data + ROM.misc.size, ssize);

		FCEU_free(ROM.prg.data);

		ROM.prg.size = ssize;
		ROM.prg.data = (uint8 *)FCEU_malloc(ssize);

		memcpy(ROM.prg.data, buffer, ssize);

		/* mount new PRG-ROM and Misc ROM */
		SetupCartPRGMapping(0, ROM.prg.data, ROM.prg.size, FALSE);
		SetupCartPRGMapping(1, ROM.misc.data, ROM.misc.size, FALSE);

		FCEU_free(buffer);
	}

	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
