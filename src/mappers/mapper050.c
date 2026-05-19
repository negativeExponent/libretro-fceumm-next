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
	uint8_t reg;
	uint32_t IRQCount, IRQa;
} m050;

static SFORMAT StateRegs[] = {
	{ &m050.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m050.IRQa, 4 | FCEUSTATE_RLSB, "IRQA" },
	{ &m050.reg, 1, "REG" },
	{ 0 }
};

static void Sync(void) {
	uint8_t prg = ((m050.reg & 0x01) << 2) | ((m050.reg & 0x02) >> 1) | ((m050.reg & 0x04) >> 1) | (m050.reg & 0x08);

	setprg8(0x6000, 0x0F);
	setprg8(0x8000, 0x08);
	setprg8(0xA000, 0x09);
	setprg8(0xC000, prg);
	setprg8(0xE000, 0x0B);
	setchr8(0);
}

static DECLFW(WriteReg) {
	switch (A & 0xD160) {
	case 0x4120:
		m050.IRQa = V & 0x01;
		if (!m050.IRQa) {
			m050.IRQCount = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 0x4020:
		m050.reg = V;
		Sync();
		break;
	}
}

static void Power(void) {
	memset(&m050, 0, sizeof(m050));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0x4FFF, WriteReg);
}

static void CPUIRQHook(int a) {
	if (m050.IRQa) {
		m050.IRQCount += a;
		if (m050.IRQCount & 0x1000) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper050_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
