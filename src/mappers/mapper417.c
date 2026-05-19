/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t nt[4];
	uint8_t IRQa;
	uint16_t IRQCount;
} m417;

static SFORMAT StateRegs[] = {
	{ m417.prg, 4, "PREG" },
	{ m417.chr, 8, "CREG" },
	{ m417.nt, 4, "NREG" },
	{ &m417.IRQa, 1, "IRQA" },
	{ &m417.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m417.prg[0]);
	setprg8(0xA000, m417.prg[1]);
	setprg8(0xC000, m417.prg[2]);
	setprg8(0xE000, 0xFF);
}

static void SyncCHR(void) {
	setchr1(0x0000, m417.chr[0]);
	setchr1(0x0400, m417.chr[1]);
	setchr1(0x0800, m417.chr[2]);
	setchr1(0x0C00, m417.chr[3]);
	setchr1(0x1000, m417.chr[4]);
	setchr1(0x1400, m417.chr[5]);
	setchr1(0x1800, m417.chr[6]);
	setchr1(0x1C00, m417.chr[7]);
}

static void SyncMirror(void) {
	setmirrorw(m417.nt[0] & 0x01, m417.nt[1] & 0x01, m417.nt[2] & 0x01, m417.nt[3] & 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0x8073) {
	case 0x8000:
	case 0x8001:
	case 0x8002:
	case 0x8003:
		m417.prg[A & 0x03] = V;
		SyncPRG();
		break;
	case 0x8010:
	case 0x8011:
	case 0x8012:
	case 0x8013:
		m417.chr[0 | (A & 0x03)] = V;
		if (iNESCart.submapper == 1) {
			m417.nt[A & 0x03] = V >> 7;
		}
		SyncCHR();
		SyncMirror();
		break;
	case 0x8020:
	case 0x8021:
	case 0x8022:
	case 0x8023:
		m417.chr[0x04 | (A & 0x03)] = V;
		SyncCHR();
		break;
	case 0x8030:
		m417.IRQCount = 0;
		m417.IRQa = TRUE;
		break;
	case 0x8040:
		m417.IRQa = FALSE;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x8050:
	case 0x8051:
	case 0x8052:
	case 0x8053:
		if (iNESCart.submapper == 0) {
			m417.nt[A & 0x03] = V;
			SyncMirror();
		}
		break;
	}
}

static void CPUIRQHook(int a) {
	uint16_t mask = (iNESCart.submapper == 1) ? 0x1000 : 0x400;

	m417.IRQCount += a;
	if (m417.IRQa && (m417.IRQCount & mask)) {
		X6502_IRQBegin(FCEU_IQEXT);
		m417.IRQCount = 0;
	}
}

static void Power(void) {
	memset(&m417, 0, sizeof(m417));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper417_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
