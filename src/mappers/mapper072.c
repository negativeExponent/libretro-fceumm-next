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
 *
 * Mapper 72:
 * Moero!! Pro Tennis have ADPCM codec on-board, PROM isn't dumped, emulation isn't
 * possible just now.
 *
 * Mapper 092
 * Another two-in-one mapper, two Jaleco carts uses similar
 * hardware, but with different wiring.
 * Original code provided by LULU
 * Additionally, PCB contains DSP extra sound chip, used for voice samples (unemulated)
 * This mapper is identical to mapper 072 except for the different PRG Setup.
 *
 * TODO: Speech support
 */

#include "mapinc.h"

static struct {
	uint8_t prg;
	uint8_t chr;
	uint8_t reg;
} m072;

static SFORMAT StateRegs[] = {
	{ &m072.prg, 1, "PREG" },
	{ &m072.chr, 1, "CREG" },
	{ &m072.reg, 1, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	if (iNESCart.mapper == 92) {
		setprg16(0x8000, 0);
		setprg16(0xC000, m072.prg);
	} else {
		setprg16(0x8000, m072.prg);
		setprg16(0xC000, ~0);
	}
}

static void SyncCHR(void) {
	setchr8(m072.chr);
}

static DECLFW(WriteReg) {
	V &= CartBR(A); /* bus conflict */

	m072.reg = (m072.reg ^ V) & V;
	if (m072.reg & 0x80) {
		m072.prg = V;
		SyncPRG();
	}
	if (m072.reg & 0x40) {
		m072.chr = V;
		SyncCHR();
	}
}

static void Power(void) {
	memset(&m072, 0, sizeof(m072));
	SyncPRG();
	SyncCHR();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
}

void Mapper072_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
