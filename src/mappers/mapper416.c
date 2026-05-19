/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t smb2j_reg;
	uint8_t IRQa;
	uint16_t IRQCount;
} m416;

static SFORMAT StateRegs[] = {
	{ &m416.smb2j_reg, 1, "SMB2" },
	{ &m416.IRQa, 1, "IRQA" },
	{ &m416.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = ((latch.data >> 1) & 0x04) | ((latch.data >> 6) & 0x02) | ((latch.data >> 5) & 0x01);

	setprg8(0x6000, 0x07);
	if (latch.data & 0x08) {
		switch (latch.data & 0xC0) {
		case 0x00:
			setprg8(0x8000, bank << 1);
			setprg8(0xA000, bank << 1);
			setprg8(0xC000, bank << 1);
			setprg8(0xE000, bank << 1);
			break;
		case 0x40:
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
			break;
		case 0x80:
		case 0xC0:
			setprg32(0x8000, bank >> 1);
			break;
		}
	} else {
		setprg8(0x8000, 0x00);
		setprg8(0xA000, 0x01);
		setprg8(0xC000, (m416.smb2j_reg & ~0x07) | ((m416.smb2j_reg << 2) & 0x04) | ((m416.smb2j_reg >> 1) & 0x03));
		setprg8(0xE000, 0x03);
	}
	setchr8((latch.data >> 1) & 0x03);
	setmirror(((latch.data >> 2) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0x4160) {
	case 0x4120:
		m416.IRQa = V;
		if (!m416.IRQa) {
			m416.IRQCount = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 0x4020:
		m416.smb2j_reg = V;
		Sync();
		break;
	}
}

static void CPUIRQHook(int a) {
	if (m416.IRQa & 0x01) {
		m416.IRQCount += a;
		if (m416.IRQCount & 0x1000) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Reset(void) {
	BWrite[0x0163](0x0163, 0);
	memset(&m416, 0, sizeof(m416));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m416, 0, sizeof(m416));
	Latch_Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x4020, 0x5FFF, WriteReg);
	SetWriteHandler(0xA000, 0xFFFF, 0);
}

void Mapper416_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
