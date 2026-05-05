/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
 * NES 2.0 Mapper 330 is used for a bootleg version of Contra/Gryzor.
 * as implemented from
 * http://forums.nesdev.org/viewtopic.php?f=9&t=17352&p=218722#p218722
 */

#include "mapinc.h"
#include "n163sound.h"

static struct {
	uint8_t prg[4], chr[8], nmt[4];
	uint8_t IRQa;
	uint16_t IRQCount;
} m330;

static uint8_t internalRAM[128];

static SFORMAT StateRegs[] = {
	{ m330.prg, 4, "PREG" },
	{ m330.chr, 8, "CREG" },
	{ m330.nmt, 4, "NREG" },
	{ &m330.IRQa, 1, "IRQA" },
	{ &m330.IRQCount, 2, "IRQC" },
	{ 0 }
};

static void SyncPRG(void) {
	int i;

	for (i = 0; i < 4; i++) {
		setprg8(0x8000 + (i * 0x2000), m330.prg[i]);
	}
}

static void SyncCHR(void) {
	int i;

	for (i = 0; i < 8; i++) {
		setchr1(i * 0x400, m330.chr[i]);
	}
}

static void SyncNMT(void) {
	int i;

	for (i = 0; i < 4; i++) {
		setntamem(NTARAM + (m330.nmt[i] * 0x400), TRUE, i);
	}
}

static DECLFW(WriteCHR) {
	if ((A & 0x400) && !(A & 0x4000)) {
		if (A & 0x2000) {
			m330.IRQCount &= 0x00FF;
			m330.IRQCount |= (V & 0x7F) << 8;
			m330.IRQa = V & 0x80;
			X6502_IRQEnd(FCEU_IQEXT);
		} else {
			m330.IRQCount &= 0xFF00;
			m330.IRQCount |= V;
		}
	} else {
		m330.chr[(A >> 11) & 0x07] = V;
		SyncCHR();
	}
}

static DECLFW(WriteNMT) {
	if (!(A & 0x400)) {
		int index = (A >> 11) & 0x03;
		m330.nmt[index] = V;
		SyncNMT();
	}
}

static DECLFW(WritePRG) {
	if ((A >= 0xF000) && (A & 0x800)) {
		N163Sound_Write(A, V);
	} else if (!(A & 0x400)) {
		m330.prg[(A >> 11) & 0x03] = V;
		SyncPRG();
	}
}

static void Power(void) {
	int i;

	for (i = 0; i < 4; i++) {
		m330.prg[i] = i;
	}
	m330.prg[3] = ~0;
	for (i = 0; i < 8; i++) {
		m330.chr[i] = i;
	}
	for (i = 0; i < 4; i++) {
		m330.nmt[i] = (i >> 1) & 0x01;
	}

	m330.IRQa = m330.IRQCount = 0;

	SyncPRG();
	SyncCHR();
	SyncNMT();
	setprg8r(0x10, 0x6000, 0);

	SetReadHandler(0x4800, 0x4FFF, N163Sound_Read);
	SetWriteHandler(0x4800, 0x4FFF, N163Sound_Write);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);

	SetWriteHandler(0x8000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xDFFF, WriteNMT);
	SetWriteHandler(0xE000, 0xFFFF, WritePRG);
}

static void CPUIRQHook(int a) {
	if (m330.IRQa) {
		m330.IRQCount += a;
		if (m330.IRQCount > 0x7FFF) {
			X6502_IRQBegin(FCEU_IQEXT);
			m330.IRQa = 0;
			m330.IRQCount = 0;
		}
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncNMT();
}

void Mapper330_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAM = (uint8_t *)FCEU_gmalloc(8192);
	SetupCartPRGMapping(0x10, WRAM, 8192, 1);
	AddExState(WRAM, 8192, 0, "WRAM");

	N163Sound_ESI(internalRAM);
	N163Sound_AddStateInfo();
}
