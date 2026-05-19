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

/* iNES Mapper 018 represents the Jaleco SS 88006 mapper used for Magic John
   (Japanese version of Totally Rad) and about a dozen other games. */

#include "mapinc.h"

static struct  {
	uint8_t prg[4], chr[8];
	uint8_t IRQa, mirr;
	int32_t IRQCount, IRQLatch;
} m018;

static SFORMAT StateRegs[] = {
	{ m018.prg, 4, "PREG" },
	{ m018.chr, 8, "CREG" },
	{ &m018.mirr, 1, "MIRR" },
	{ &m018.IRQa, 1, "IRQA" },
	{ &m018.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m018.IRQLatch, 4 | FCEUSTATE_RLSB, "IRQL" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg8(0x8000, m018.prg[0]);
	setprg8(0xA000, m018.prg[1]);
	setprg8(0xC000, m018.prg[2]);
	setprg8(0xE000, ~0);
}

static void SyncCHR(void) {
	setchr1(0x0000, m018.chr[0]);
	setchr1(0x0400, m018.chr[1]);
	setchr1(0x0800, m018.chr[2]);
	setchr1(0x0C00, m018.chr[3]);
	setchr1(0x1000, m018.chr[4]);
	setchr1(0x1400, m018.chr[5]);
	setchr1(0x1800, m018.chr[6]);
	setchr1(0x1C00, m018.chr[7]);
}

static void SyncMirror(void) {
	switch (m018.mirr & 0x03) {
	case 0:
		setmirror(MI_H);
		break;
	case 1:
		setmirror(MI_V);
		break;
	case 2:
		setmirror(MI_0);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

static DECLFR(ReadWRAM) {
	if (!(m018.prg[3] & 0x01)) {
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteWRAM) {
	if (!(m018.prg[3] & 0x01) || !(m018.prg[3] & 0x02)) {
		return;
	}
	CartBW(A, V);
}

static DECLFW(WritePrg) {
	uint8_t index = ((A >> 1) & 1) | ((A - 0x8000) >> 11);

	if (A & 0x01) {
		m018.prg[index] = (m018.prg[index] & 0x0F) | (V << 4);
	} else {
		m018.prg[index] = (m018.prg[index] & 0xF0) | (V & 0x0F);
	}
	SyncPRG();
}

static DECLFW(WriteChr) {
	uint8_t index = ((A >> 1) & 1) | ((A - 0xA000) >> 11);

	if (A & 0x01) {
		m018.chr[index] = (m018.chr[index] & 0x0F) | (V << 4);
	} else {
		m018.chr[index] = (m018.chr[index] & 0xF0) | (V & 0x0F);
	}
	SyncCHR();
}

static DECLFW(WriteLatch) {
	switch (A & 0x03) {
	case 0:
		m018.IRQLatch = (m018.IRQLatch & 0xFFF0) | ((V & 0x0F) << 0);
		break;
	case 1:
		m018.IRQLatch = (m018.IRQLatch & 0xFF0F) | ((V & 0x0F) << 4);
		break;
	case 2:
		m018.IRQLatch = (m018.IRQLatch & 0xF0FF) | ((V & 0x0F) << 8);
		break;
	case 3:
		m018.IRQLatch = (m018.IRQLatch & 0x0FFF) | ((V & 0x0F) << 12);
		break;
	}
}

static DECLFW(WriteMisc) {
	switch (A & 0xF003) {
	case 0xF000:
		m018.IRQCount = m018.IRQLatch;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xF001:
		m018.IRQa = V;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xF002:
		m018.mirr = V;
		SyncMirror();
		break;
	case 0xF003:
		break;
	}
}

static void Power(void) {
	memset(&m018, 0, sizeof(m018));

	m018.prg[0] = 0;
	m018.prg[1] = 1;
	m018.prg[2] = ~1;
	m018.prg[3] = 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x6000, 0x7FFF, ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, WriteWRAM);

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, WritePrg);
	SetWriteHandler(0xA000, 0xDFFF, WriteChr);
	SetWriteHandler(0xE000, 0xEFFF, WriteLatch);
	SetWriteHandler(0xF000, 0xFFFF, WriteMisc);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void CPUCycle(int a) {
	if (m018.IRQa & 0x01) {
		uint16_t mask = 0xFFFF;

		if (m018.IRQa & 0x08) {
			mask = 0x000F;
		} else if (m018.IRQa & 0x04) {
			mask = 0x00FF;
		} else if (m018.IRQa & 0x02) {
			mask = 0x0FFF;
		}

		while (a--) {
			if ((m018.IRQCount & mask) && !(--m018.IRQCount & mask)) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper018_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}
