/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

#include "mapinc.h"

static struct {
	uint8 prg[2], chr[8], mirror, cmd;
	uint8 IRQa;
	int16 IRQCount, IRQLatch;
} m065;

static SFORMAT StateRegs[] = {
	{ &m065.cmd, 1, "CMD0" },
	{ m065.prg, 2, "PREG" },
	{ m065.chr, 8, "CREG" },
	{ &m065.mirror, 1, "MIRR" },
	{ &m065.IRQa, 1, "IRQA" },
	{ &m065.IRQCount, 2, "IRQC" },
	{ &m065.IRQLatch, 2, "IRQL" },
	{ 0 }
};

static void SyncPRG(void) {
	uint16 pswap = (m065.cmd & 0x80) ? 0x4000 : 0;

	setprg8(0x8000 ^ pswap, m065.prg[0]);
	setprg8(0xA000, m065.prg[1]);
	setprg8(0xC000 ^ pswap, 0xFE);
	setprg8(0xE000, 0xFF);
}

static void SyncCHR(void) {
	setchr1(0x0000, m065.chr[0]);
	setchr1(0x0400, m065.chr[1]);
	setchr1(0x0800, m065.chr[2]);
	setchr1(0x0C00, m065.chr[3]);
	setchr1(0x1000, m065.chr[4]);
	setchr1(0x1400, m065.chr[5]);
	setchr1(0x1800, m065.chr[6]);
	setchr1(0x1C00, m065.chr[7]);
}

static void SyncMirror(void) {
	switch (m065.mirror >> 6) {
	case 0:
		setmirror(MI_V);
		break;
	case 2:
		setmirror(MI_H);
		break;
	default:
		setmirror(MI_0);
		break;
	}
}

static DECLFW(WritePRG) {
	m065.prg[(A >> 13) & 0x01] = V;
	SyncPRG();
}

static DECLFW(WriteMisc) {
	switch (A & 0x07) {
	case 0:
		m065.cmd = V;
		SyncPRG();
		break;
	case 1:
		m065.mirror = V;
		SyncMirror();
		break;
	case 3:
		m065.IRQa = V & 0x80;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 4:
		m065.IRQCount = m065.IRQLatch;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 5:
		m065.IRQLatch &= 0x00FF;
		m065.IRQLatch |= V << 8;
		break;
	case 6:
		m065.IRQLatch &= 0xFF00;
		m065.IRQLatch |= V;
		break;
	}
}

static DECLFW(WriteCHR) {
	m065.chr[A & 0x07] = V;
	SyncCHR();
}

static void CPUIRQHook(int a) {
	if (m065.IRQa) {
		m065.IRQCount -= a;
		if (m065.IRQCount <= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			m065.IRQa = 0;
		}
	}
}

static void Power(void) {
	memset(&m065, 0, sizeof(m065));

	m065.prg[0] = 0;
	m065.prg[1] = 1;

	m065.chr[0] = 0;
	m065.chr[1] = 1;
	m065.chr[2] = 2;
	m065.chr[3] = 3;
	m065.chr[4] = 4;
	m065.chr[5] = 5;
	m065.chr[6] = 6;
	m065.chr[7] = 7;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, WriteMisc);
	SetWriteHandler(0xA000, 0xAFFF, WritePRG);
	SetWriteHandler(0xB000, 0xBFFF, WriteCHR);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper065_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
