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

#include "mapinc.h"

static struct {
	uint8_t prg[4], chr[8], nt[4];
	uint16_t IRQCount, IRQa;
} m127;

static SFORMAT StateRegs[] = {
	{ m127.prg, 4, "PREG" },
	{ m127.chr, 8, "CREG" },
	{ m127.nt, 4, "NTAM" },
	{ &m127.IRQa, 1, "IRQA" },
	{ &m127.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void SyncPRG(void) {
	m127.prg[3] |= 0x0C;
	setprg8(0x8000, m127.prg[0] & 0x0F);
	setprg8(0xA000, m127.prg[1] & 0x0F);
	setprg8(0xC000, m127.prg[2] & 0x0F);
	setprg8(0xE000, m127.prg[3] & 0x0F);
}

static void SyncCHR(void) {
	setchr1(0x0000, m127.chr[0] & 0x7F);
	setchr1(0x0400, m127.chr[1] & 0x7F);
	setchr1(0x0800, m127.chr[2] & 0x7F);
	setchr1(0x0C00, m127.chr[3] & 0x7F);
	setchr1(0x1000, m127.chr[4] & 0x7F);
	setchr1(0x1400, m127.chr[5] & 0x7F);
	setchr1(0x1800, m127.chr[6] & 0x7F);
	setchr1(0x1C00, m127.chr[7] & 0x7F);
}

static void SyncMirror(void) {
	setmirrorw(m127.nt[0] & 0x01, m127.nt[1] & 0x01, m127.nt[2] & 0x01, m127.nt[3] & 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0x73) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		m127.prg[A & 3] = V;
		SyncPRG();
		break;

	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
		m127.chr[((A >> 3) & 4) | (A & 3)] = V;
		SyncCHR();
		break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
		m127.IRQa = TRUE;
		break;

	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		m127.IRQa = FALSE;
		m127.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;

	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
		m127.nt[A & 3] = V;
		SyncMirror();
		break;
	}
}

static void Power(void) {
	memset(&m127, 0, sizeof(m127));

	m127.prg[0] = 0x0F;
	m127.prg[1] = 0x0F;
	m127.prg[2] = 0x0F;
	m127.prg[3] = 0x0F;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void CPUIRQHook(int a) {
	if (m127.IRQa) {
		m127.IRQCount += a;
		if (m127.IRQCount & 0x100) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper127_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
