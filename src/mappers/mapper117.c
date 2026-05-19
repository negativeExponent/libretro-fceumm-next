/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
	uint8_t prg[4], chr[8], mirror;
	uint8_t IRQa, IRQm, IRQLatch, IRQCount;
} m117;

static SFORMAT StateRegs[] = {
	{ m117.prg, 4, "PREG" },
	{ m117.chr, 8, "CREG" },
	{ &m117.mirror, 1, "MREG" },
	{ &m117.IRQa, 1, "IRQA" },
	{ &m117.IRQCount, 1, "IRQC" },
	{ &m117.IRQLatch, 1, "IRQL" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m117.prg[0]);
	setprg8(0xA000, m117.prg[1]);
	setprg8(0xC000, m117.prg[2]);
	setprg8(0xE000, ~0);
}

static void SyncCHR(void) {
	setchr1(0x0000, m117.chr[0]);
	setchr1(0x0400, m117.chr[1]);
	setchr1(0x0800, m117.chr[2]);
	setchr1(0x0C00, m117.chr[3]);
	setchr1(0x1000, m117.chr[4]);
	setchr1(0x1400, m117.chr[5]);
	setchr1(0x1800, m117.chr[6]);
	setchr1(0x1C00, m117.chr[7]);
}

static void SyncMirror(void) {
	setmirror(m117.mirror ^ 1);
}

static DECLFW(WritePRG) {
	m117.prg[A & 0x03] = V;
	SyncPRG();
}

static DECLFW(WriteCHR) {
	if (A & 0x08) {
		/* Unknown, writes should be ignored */
	} else {
		m117.chr[A & 0x07] = V;
		SyncCHR();
	}
}

static DECLFW(WriteIRQ) {
	switch (A & 0x03) {
	case 1:
		m117.IRQLatch = V;
		break;
	case 3:
		m117.IRQCount = m117.IRQLatch;
		m117.IRQa |= 2;
		break;
	}
	X6502_IRQEnd(FCEU_IQEXT);
}

static DECLFW(WriteMirror) {
	m117.mirror = V;
	SyncMirror();
}

static DECLFW(WriteIRQMode) {
	m117.IRQa &= ~1;
	m117.IRQa |= V & 1;
	X6502_IRQEnd(FCEU_IQEXT);
}

static void HBIRQHook(void) {
	if (m117.IRQa == 3 && m117.IRQCount) {
		m117.IRQCount--;
		if (!m117.IRQCount) {
			m117.IRQa &= 1;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m117, 0, sizeof(m117));

	m117.prg[0] = 0xFC;
	m117.prg[1] = 0xFD;
	m117.prg[2] = 0xFE;
	m117.prg[3] = 0xFF;

	m117.chr[0] = 0x00;
	m117.chr[1] = 0x01;
	m117.chr[2] = 0x02;
	m117.chr[3] = 0x03;
	m117.chr[4] = 0x04;
	m117.chr[5] = 0x05;
	m117.chr[6] = 0x06;
	m117.chr[7] = 0x07;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0xA000, 0xAFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xCFFF, WriteIRQ);
	SetWriteHandler(0xD000, 0xDFFF, WriteMirror);
	SetWriteHandler(0xE000, 0xEFFF, WriteIRQMode);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper117_Init(CartInfo *info) {
	info->Power = Power;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
