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
 *
 * FDS Conversion
 *
 */

#include "mapinc.h"

static struct {
	uint8_t prg;
	uint8_t outer;
	uint32_t IRQCount, IRQa;
} m040;

static SFORMAT StateRegs[] = {
	{ &m040.prg, 1, "PREG" },
	{ &m040.outer, 1, "OUTB" },
	{ &m040.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m040.IRQa, 4 | FCEUSTATE_RLSB, "IRQA" },
	{ 0 }
};

static void Sync(void) {
	if (m040.outer & 0x08) {
		if (m040.outer & 0x10) {
			setprg32(0x8000, 2 | (m040.outer >> 6));
		} else {
			setprg16(0x8000, 4 | (m040.outer >> 5));
			setprg16(0xC000, 4 | (m040.outer >> 5));
		}
	} else {
		setprg8(0x6000, 6);
		setprg8(0x8000, 4);
		setprg8(0xA000, 5);
		setprg8(0xC000, m040.prg & 0x07);
		setprg8(0xE000, 7);
	}
	setchr8((m040.outer >> 1) & 0x03);
	setmirror((m040.outer & 0x01) ^ 0x01);
}

static DECLFW(WriteIRQ) {
	m040.IRQa = (A >> 13) & 0x01;
	m040.IRQCount = 0;
	X6502_IRQEnd(FCEU_IQEXT);
}

static DECLFW(WriteOuter) {
	m040.outer = A & 0xFF;
	Sync();
}

static DECLFW(WritePRG) {
	m040.prg = V;
	Sync();
}

static void CPUIRQHook(int a) {
	if (m040.IRQa) {
		m040.IRQCount += a;
		if (m040.IRQCount & 0x1000) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m040, 0, sizeof(m040));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteIRQ);
	if (iNESCart.submapper == 1) {
		SetWriteHandler(0xC000, 0xDFFF, WriteOuter);
	}
	SetWriteHandler(0xE000, 0xFFFF, WritePRG);
}

static void Reset(void) {
	memset(&m040, 0, sizeof(m040));
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper040_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
