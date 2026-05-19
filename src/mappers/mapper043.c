/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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
	uint8_t IRQa;
	uint16_t IRQCount;
} m043;

static int prgBankOrder[8] = { 4, 3, 4, 4, 4, 7, 5, 6 };
static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m043.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &m043.IRQa, 1, "IRQA" },
	{ &m043.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	int bank = (0x10000 | (0x800 * dipsw)) / 2048;

	setprg2(0x5000, bank); /* Only YS-612 advanced version, 2 KiB PRG-ROM bank, repeated once, from 2 KiB PRG-ROM chip */
	setprg2(0x5800, bank);

	setprg8(0x6000, 0x02);

	setprg8(0x8000, 0x01);
	setprg8(0xA000, 0x00);
	setprg8(0xC000, prgBankOrder[m043.reg & 0x07]);
	setprg8(0xE000, 0x09);

	setchr8(0);
}

static DECLFW(WriteReg) {
	switch (A & 0xF1FF) {
	case 0x4022:
		m043.reg = V;
		setprg8(0xC000, prgBankOrder[V & 0x07]);
		break;
	case 0x8122: /* 0x8122 - hacked version */
	case 0x4122: /* 0x4122 - original version */
		m043.IRQa = V;
		if (V & 0x02) {
			m043.IRQCount = 0;
		}
		if (!(m043.IRQa & 0x01)) {
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	}
}

static void Reset(void) {
	dipsw = (dipsw + 1) & 0x03;
	Sync();
	FCEU_printf("dipswitch = %d\n", dipsw);
}

static void Power(void) {
	memset(&m043, 0, sizeof(m043));
	dipsw = 1;
	Sync();
	SetReadHandler(0x5000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0x8FFF, WriteReg);
}

static void CPUIRQHook(int a) {
	m043.IRQCount += a;
	if (m043.IRQCount >= 4096) {
		if (m043.IRQa & 0x01) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper043_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
