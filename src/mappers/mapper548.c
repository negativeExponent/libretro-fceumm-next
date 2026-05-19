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

/*
 * NES 2.0 Mapper 548 denotes 科統實業股份有限公司 (Co Tung Co.)'s CTC-15
 * circuit board, used for their cartridge conversion of the FDS game
 * Almanaの奇跡 (Almana no Kiseki).
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t reg, latch, IRQa;
	uint16_t IRQCount;
} m548;

static SFORMAT StateRegs[] = {
	{ &m548.reg, 1, "REG" },
	{ &m548.latch, 1, "LATC" },
	{ &m548.IRQa, 1, "IRQA" },
	{ &m548.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, m548.reg);
	setprg16(0xC000, 0x03);
}

static DECLFW(WriteLatch) {
	m548.latch = ((A >> 3) & 0x04) | ((A >> 2) & 0x03);
	m548.IRQa = (A & 0x04) != 0x04;
	if (!m548.IRQa) {
		m548.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	}
}

static DECLFW(ApplyLatch) {
	m548.reg = m548.latch ^ 0x05;
	Sync();
}

static void CPUIRQHook(int a) {
	/*
	 * If counting is enabled, the counter is clocked on every falling edge of
	 * M2. IRQ is asserted while ((counter÷640)&37)=37, so it is asserted the
	 * first time when the counter reaches 23680 and self-acknowledges the first
	 * time when it reaches 24320.
	 */
	int curcount = m548.IRQCount;
	int newCount = m548.IRQCount + a;

	if (m548.IRQa) {
		if (curcount >= 24320) {
			X6502_IRQEnd(FCEU_IQEXT);
			m548.IRQa = 0;
		} else {
			if ((curcount < 23680) && newCount >= 23680) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
		m548.IRQCount = newCount;
	}
}

static void Power(void) {
	memset(&m548, 0, sizeof(m548));
	m548.latch = 7;
	m548.reg = m548.latch ^ 0x05;
	FDSSound_Power();
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4800, 0x4FFF, WriteLatch);
	SetWriteHandler(0x5000, 0x57FF, ApplyLatch);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper548_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
