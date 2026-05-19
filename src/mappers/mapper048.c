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
 */

/* iNES Mapper 48 - Taito TC0690/TC190+PAL16R4 */

#include "mapinc.h"

static struct {
	uint8_t prg[2], chr[6], mirror;
	uint8_t IRQCount, IRQLatch, IRQa;
	uint8_t IRQReload;
	int8_t IRQDelay;
} m048;

static uint8_t isFlintStones = FALSE;

static SFORMAT StateRegs[] = {
	{ m048.prg, 2, "PREG" },
	{ m048.chr, 6, "CREG" },
	{ &m048.mirror, 1, "MIRR" },
	{ &m048.IRQCount, 1, "IRQC" },
	{ &m048.IRQLatch, 1, "IRQL" },
	{ &m048.IRQa, 1, "IRQA" },
	{ &m048.IRQDelay, 1, "IRQD" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m048.prg[0]);
	setprg8(0xA000, m048.prg[1]);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr2(0x0000, m048.chr[0]);
	setchr2(0x0800, m048.chr[1]);
	setchr1(0x1000, m048.chr[2]);
	setchr1(0x1400, m048.chr[3]);
	setchr1(0x1800, m048.chr[4]);
	setchr1(0x1C00, m048.chr[5]);
}

static void SyncMirror(void) {
	setmirror(((m048.mirror >> 6) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0xE003) {
	case 0x8000:
	case 0x8001:
		m048.prg[A & 0x01] = V;
		SyncPRG();
		break;
	case 0x8002:
	case 0x8003:
		m048.chr[A & 0x01] = V;
		SyncCHR();
		break;
	case 0xA000:
	case 0xA001:
	case 0xA002:
	case 0xA003:
		m048.chr[2 + (A & 0x03)] = V;
		SyncCHR();
		break;
	}
}

static DECLFW(WriteIRQ) {
	switch (A & 0xE003) {
	case 0xC000:
		X6502_IRQEnd(FCEU_IQEXT);
		m048.IRQLatch = V ^ 0xFF;
		break;
	case 0xC001:
		X6502_IRQEnd(FCEU_IQEXT);
		m048.IRQCount = 0;
		m048.IRQReload = TRUE;
		break;
	case 0xC002:
		m048.IRQa = TRUE;
		break;
	case 0xC003:
		X6502_IRQEnd(FCEU_IQEXT);
		m048.IRQa = FALSE;
		break;
	}
}

static DECLFW(WriteMirror) {
	m048.mirror = V;
	SyncMirror();
}

static void CPUCycle(int a) {
	if (m048.IRQDelay > 0) {
		m048.IRQDelay -= a;
		if (m048.IRQDelay <= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void ScanlineCounter(void) {
	int count = m048.IRQCount;
	if (!count || m048.IRQReload) {
		m048.IRQCount = m048.IRQLatch;
	} else {
		m048.IRQCount--;
	}
	if (!m048.IRQCount && m048.IRQa) {
		/* Flintstons has a much later irq triggering */
		m048.IRQDelay = isFlintStones ? 19 : 7;
	}
	m048.IRQReload = FALSE;
}

static void Power(void) {
	memset (&m048, 0, sizeof(m048));

	m048.prg[0] = 0x00;
	m048.prg[1] = 0x01;

	m048.chr[0] = 0x00;
	m048.chr[1] = 0x01;
	m048.chr[2] = 0x04;
	m048.chr[3] = 0x05;
	m048.chr[4] = 0x06;
	m048.chr[5] = 0x07;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteReg);
	SetWriteHandler(0xC000, 0xDFFF, WriteIRQ);
	SetWriteHandler(0xE000, 0xFFFF, WriteMirror);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper048_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	MapIRQHook = CPUCycle;
	GameHBIRQHook = ScanlineCounter;
	isFlintStones = (info->CRC32 == 0x40C0AD47) ? TRUE : FALSE;
}
