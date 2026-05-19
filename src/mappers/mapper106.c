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
 */

#include "mapinc.h"

static struct {
	uint8_t prg[4], chr[8];
	uint8_t mirror, IRQa;
	uint32_t IRQCount;
} m106;

static SFORMAT StateRegs[] = {
	{ m106.prg, 4, "PREGS" },
	{ m106.chr, 8, "CREGS" },
	{ &m106.IRQa, 1, "IRQA" },
	{ &m106.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg8(0x8000, (m106.prg[0] & 0x0F) | 0x10);
	setprg8(0xA000, (m106.prg[1] & 0x1F));
	setprg8(0xC000, (m106.prg[2] & 0x1F));
	setprg8(0xE000, (m106.prg[3] & 0x0F) | 0x10);
}

static void SyncCHR(void) {
	setchr1(0x0000, m106.chr[0] & ~1);
	setchr1(0x0400, m106.chr[1] | 1);
	setchr1(0x0800, m106.chr[2] & ~1);
	setchr1(0x0c00, m106.chr[3] | 1);
	setchr1(0x1000, m106.chr[4]);
	setchr1(0x1400, m106.chr[5]);
	setchr1(0x1800, m106.chr[6]);
	setchr1(0x1C00, m106.chr[7]);
}

static void SyncMirror(void) {
	setmirror((m106.mirror & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0x0F) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		m106.chr[A & 0x07] = V;
		SyncCHR();
		break;
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
		m106.prg[A & 0x03] = V;
		SyncPRG();
		break;
	case 0x0C:
		m106.mirror = V;
		SyncMirror();
		break;
	case 0x0D:
		m106.IRQa = 0;
		m106.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x0E:
		m106.IRQCount = (m106.IRQCount & 0xFF00) | V;
		break;
	case 0x0F:
		m106.IRQCount = (m106.IRQCount & 0x00FF) | (V << 8);
		m106.IRQa = 1;
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m106.IRQCount < 0xFFFF) {
		m106.IRQCount += a;
		if (m106.IRQCount >= 0xFFFF) {
			if (m106.IRQa) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void Power(void) {
	memset(&m106, 0, sizeof(m106));

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper106_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
