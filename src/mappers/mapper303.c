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
 * NES 2.0 Mapper 303 - Kaiser 7017
 * UNIF UNL-KS7017
 * FDS Conversion - Almana No Kiseki
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t prg, mirror;
	int32_t IRQa, IRQCount, IRQLatch;
} m303;

static SFORMAT StateRegs[] = {
	{ &m303.mirror, 1, "MIRR" },
	{ &m303.prg, 1, "REGS" },
	{ &m303.IRQa, 4, "IRQA" },
	{ &m303.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m303.IRQLatch, 4 | FCEUSTATE_RLSB, "IRQL" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg16(0x8000, m303.prg);
	setprg16(0xC000, 0x02);
}

static void SyncMirror(void) {
	setmirror(((m303.mirror & 0x08) >> 3) ^ 0x01);
}

static DECLFR(ReadStatus) {
	/* Identical to their respective equivalents on the Famicom Disk System. */
	uint8_t ret = (cpu.IRQlow & FCEU_IQEXT) ? 1 : 0;
	X6502_IRQEnd(FCEU_IQEXT);
	return ret;
}

static DECLFW(WritePRGLatch) {
	/* The new PRG bank number is not applied until register $5100 is written to. */
	m303.prg = ((A >> 4) & 0x04) | ((A >> 2) & 0x03);
}

static DECLFW(WritePRGLatchCommit) {
	/* this is its intended purpose */
	SyncPRG();
}

static DECLFW(WriteIRQLow) {
	X6502_IRQEnd(FCEU_IQEXT);
	m303.IRQCount &= 0xFF00;
	m303.IRQCount |= V;
}

static DECLFW(WriteIRQHigh) {
	X6502_IRQEnd(FCEU_IQEXT);
	m303.IRQCount &= 0x00FF;
	m303.IRQCount |= V << 8;
	m303.IRQa = 1;
}

static DECLFW(WriteMirror) {
	m303.mirror = V;
	SyncMirror();
}

static void CPUIRQHook(int a) {
	if (m303.IRQa) {
		m303.IRQCount -= a;
		if (m303.IRQCount <= 0) {
			m303.IRQa = 0;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m303, 0, sizeof(m303));

	setprg8r(0x10, 0x6000, 0);
	setchr8(0);

	SyncPRG();
	SyncMirror();

	FDSSound_Power();

	SetReadHandler(0x4030, 0x4030, ReadStatus);
	SetWriteHandler(0x4A00, 0x4AFF, WritePRGLatch);
	SetWriteHandler(0x5100, 0x51FF, WritePRGLatchCommit);
	SetWriteHandler(0x4020, 0x4020, WriteIRQLow);
	SetWriteHandler(0x4021, 0x4021, WriteIRQHigh);
	SetWriteHandler(0x4025, 0x4025, WriteMirror);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Reset(void) {
	SyncPRG();
	SyncMirror();
	FDSSoundRegReset();
	FDSSound_SC();
}

static void StateRestore(int version) {
	SyncPRG();
	SyncMirror();
}

void Mapper303_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
