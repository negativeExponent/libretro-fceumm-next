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
 *
 * RacerMate Challenge II
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
	uint8_t protect;
	uint8_t IRQa;
	uint16_t IRQCount;
} m168;

static SFORMAT StateRegs[] = {
	{ &m168.reg, 1, "REGS" },
	{ &m168.protect, 1, "CHRP" },
	{ &m168.IRQa, 1, "IRQA" },
	{ &m168.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	/* TODO: CHR Protect */
	setchr4r(0x10, 0x0000, 0);
	setchr4r(0x10, 0x1000, m168.reg & 0x0f);
	setprg16(0x8000, m168.reg >> 6);
	setprg16(0xc000, ~0);
}

static DECLFW(WriteReg) {
	m168.reg = V;
	Sync();
}

static DECLFW(WriteIRQ) {
	if (m168.IRQa && !(A & 0x80)) {
		m168.protect = FALSE;
		Sync();
	}
	m168.IRQa = (A & 0x80) == 0;
	if (!m168.IRQa) {
		X6502_IRQEnd(FCEU_IQEXT);
		m168.IRQCount = 0;
	}
}

static void CPUCycle(int a) {
	if (m168.IRQa) {
		m168.IRQCount += a;
		if (m168.IRQCount >= 1024) {
			m168.IRQCount -= 1024;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m168, 0, sizeof(m168));

	m168.protect = TRUE;
	Sync();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteReg);
	SetWriteHandler(0xC000, 0xFFFF, WriteIRQ);
}

static void Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper168_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8 * 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
}
