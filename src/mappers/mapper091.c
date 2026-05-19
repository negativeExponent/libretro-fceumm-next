/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2020
 *  Copyright (C) 2025-2026 negativeExponent
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

/* added 2020-2-15
 * Street Fighter 3, Mortal Kombat II, Dragon Ball Z 2, Mario & Sonic 2 (JY-016)
 * 1995 Super HIK 4-in-1 (JY-016), 1995 Super HiK 4-in-1 (JY-017)
 * submapper 1 - Super Fighter III
 * NOTE: nesdev's notes for IRQ is different that whats implemented here
 */

#include "mapinc.h"

static struct {
	uint8_t chr[4], prg[2];
	uint8_t outer;
	uint8_t mirror;

	uint8_t IRQa;
	struct {
		uint8_t IRQCount;
	} pa12;
	struct {
		uint8_t IRQPrescaler;
		int16_t IRQCount;
	} m2;
} m091;

static SFORMAT StateRegs[] = {
	{ m091.chr, 4, "CREG" },
	{ m091.prg, 2, "PREG" },
	{ &m091.IRQa, 1, "IRQA" },
	{ &m091.m2.IRQPrescaler, 1, "IRQP" },
	{ &m091.pa12.IRQCount, 1, "IRQC" },
	{ &m091.m2.IRQCount, 4 | FCEUSTATE_RLSB, "IRQ2" },
	{ &m091.outer, 1, "OUTB" },
	{ &m091.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	uint16_t base = (m091.outer << 3) & ~0x0F;

	setprg8(0x8000, (base | (m091.prg[0] & 0x0F)));
	setprg8(0xA000, (base | (m091.prg[1] & 0x0F)));
	setprg8(0xC000, (base | 0x0E));
	setprg8(0xE000, (base | 0x0F));
}

static void SyncCHR(void) {
	uint16_t base = (m091.outer << 8);

	setchr2(0x0000, (base | m091.chr[0]));
	setchr2(0x0800, (base | m091.chr[1]));
	setchr2(0x1000, (base | m091.chr[2]));
	setchr2(0x1800, (base | m091.chr[3]));
}

static void SyncMirror(void) {
	if (iNESCart.submapper == 1) {
		setmirror((m091.mirror & 0x01) ^ 0x01);
	} else {
		setmirror(iNESCart.mirror);
	}
}

static DECLFW(WriteCHRM2) {
	if (iNESCart.submapper == 1) {
		switch (A & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
			m091.chr[A & 0x03] = V;
			SyncCHR();
			break;
		case 4:
		case 5:
			m091.mirror = V;
			SyncMirror();
			break;
		case 6:
			m091.m2.IRQCount = (m091.m2.IRQCount & 0xFF00) | V;
			break;
		case 7:
			m091.m2.IRQCount = (m091.m2.IRQCount & 0x00FF) | (V << 8);
			break;
		}
	} else {
		m091.chr[A & 0x03] = V;
		SyncCHR();
	}
}

static DECLFW(WritePRGIRQ) {
	switch (A & 0x03) {
	case 0:
	case 1:
		m091.prg[A & 0x01] = V;
		SyncPRG();
		break;
	case 2:
		m091.IRQa = FALSE;
		m091.pa12.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 3:
		m091.IRQa = TRUE;
		m091.m2.IRQPrescaler = 3;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static DECLFW(WriteOuter) {
	m091.outer = A & 0xFF;
	SyncPRG();
	SyncCHR();
}

static void HBIRQHook(void) {
	if ((m091.pa12.IRQCount < 8) && m091.IRQa) {
		m091.pa12.IRQCount++;
		if (m091.pa12.IRQCount >= 8) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void CPUIRQHook(int a) {
	m091.m2.IRQPrescaler += a;
	if (m091.m2.IRQPrescaler >= 4) {
		m091.m2.IRQPrescaler -= 4;
		m091.m2.IRQCount -= 5;
		if ((m091.m2.IRQCount <= 0) && m091.IRQa) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void Power(void) {
	memset(&m091, 0, sizeof(m091));

	m091.prg[0] = 0x00;
	m091.prg[1] = 0x01;

	m091.chr[0] = 0x00;
	m091.chr[1] = 0x01;
	m091.chr[2] = 0x02;
	m091.chr[3] = 0x03;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x6FFF, WriteCHRM2);
	SetWriteHandler(0x7000, 0x7FFF, WritePRGIRQ);
	SetWriteHandler(0x8000, 0x9FFF, WriteOuter);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper091_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	if (info->submapper == 1) {
		MapIRQHook = CPUIRQHook;
	} else {
		GameHBIRQHook = HBIRQHook;
	}
}
