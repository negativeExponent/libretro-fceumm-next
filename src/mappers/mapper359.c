/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* NES 2.0 Mapper 359 - BMC-SB-5013
 * NES 2.0 Mapper 540 - UNL-82112C
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t reg[4];

	uint8_t IRQReload;
	uint8_t IRQa;
	uint8_t IRQPA12;
	uint8_t IRQAutoEnable;
	uint8_t IRQLatch;
	uint8_t IRQCount;
	int16_t IRQCount16;
} m359;

static SFORMAT StateRegs[] = {
	{ m359.prg, 4, "PREG" },
	{ m359.chr, 8, "CREG" },
	{ m359.reg, 4, "EXPR" },
	{ &m359.IRQReload, 1, "IRQL" },
	{ &m359.IRQa, 1, "IRQa" },
	{ &m359.IRQPA12, 1, "IRQp" },
	{ &m359.IRQAutoEnable, 1, "IRQe" },
	{ &m359.IRQLatch, 1, "IRQL" },
	{ &m359.IRQCount, 1, "IRQC" },
	{ &m359.IRQCount16, 2 | FCEUSTATE_RLSB, "IQ16" },
	{ 0 }
};

static void SyncPRG(void) {
	uint8_t maskLut[] = { 0x3F, 0x1F, 0x2F, 0x0F };
	uint16_t mask = maskLut[m359.reg[1] & 0x03];
	uint16_t base = (m359.reg[0] & 0x38) << 1;

	setprg8(0x6000, base | (m359.prg[3] & mask));
	setprg8(0x8000, base | (m359.prg[0] & mask));
	setprg8(0xA000, base | (m359.prg[1] & mask));
	setprg8(0xC000, base | (m359.prg[2] & mask));
	setprg8(0xE000, base | (0xFF & mask));
}

static void SyncCHR(void) {
	if (!ROM.chr.size) {
		setchr8(0);
	} else {
		if (iNESCart.mapper == 540) {
			setchr2(0x0000, m359.chr[0]);
			setchr2(0x0800, m359.chr[1]);
			setchr2(0x1000, m359.chr[6]);
			setchr2(0x1800, m359.chr[7]);
		} else {
			uint16_t mask = (m359.reg[1] & 0x40) ? 0xFF : 0x7F;
			uint16_t base = (m359.reg[3] << 7);
			int i;

			for (i = 0; i < 8; i++) {
				setchr1(i * 0x0400, base | (m359.chr[i] & mask));
			}
		}
	}
}

static void SyncMirror(void) {
	switch (m359.reg[2] & 0x03) {
	case 0:
		setmirror(MI_V);
		break;
	case 1:
		setmirror(MI_H);
		break;
	case 2:
		setmirror(MI_0);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

static DECLFW(WritePRG) {
	m359.prg[A & 0x03] = V;
	SyncPRG();
}

static DECLFW(WriteReg) {
	m359.reg[A & 0x03] = V;
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static DECLFW(WriteCHR) {
	m359.chr[((A >> 10) & 0x04) | (A & 0x03)] = V;
	SyncCHR();
}

static DECLFW(WriteIRQ) {
	switch (A & 0x03) {
	case 0:
		if (m359.IRQAutoEnable) {
			m359.IRQa = FALSE;
		}
		m359.IRQCount16 &= 0xFF00;
		m359.IRQCount16 |= V;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 1:
		if (m359.IRQAutoEnable) {
			m359.IRQa = TRUE;
		}
		m359.IRQCount16 &= 0x00FF;
		m359.IRQCount16 |= (V << 8);
		m359.IRQReload = TRUE;
		m359.IRQLatch = V;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 2:
		m359.IRQa = (V & 0x01);
		m359.IRQPA12 = (V & 0x02) >> 1;
		m359.IRQAutoEnable = (V & 0x04) >> 2;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 3:
		m359.IRQa = (V & 0x01);
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static void CPUIRQHook(int a) {
	if (!m359.IRQPA12) {
		if (m359.IRQa && m359.IRQCount16) {
			m359.IRQCount16 -= a;
			if (m359.IRQCount16 <= 0)
				X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void PPUIRQHook(void) {
	if (m359.IRQPA12) {
		if (!m359.IRQCount || m359.IRQReload) {
			m359.IRQCount = m359.IRQLatch;
		} else {
			m359.IRQCount--;
		}
		if (!m359.IRQCount && m359.IRQa) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
		m359.IRQReload = FALSE;
	}
}

static void Power(void) {
	memset(&m359, 0, sizeof(m359));
	m359.prg[0] = ~3;
	m359.prg[1] = ~2;
	m359.prg[2] = ~1;
	m359.prg[3] = ~0;
	m359.chr[0] = 0;
	m359.chr[1] = 1;
	m359.chr[2] = 2;
	m359.chr[3] = 3;
	m359.chr[4] = 4;
	m359.chr[5] = 5;
	m359.chr[6] = 6;
	m359.chr[7] = 7;

	m359.reg[1] = 0x40;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	FDSSound_Power();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, WriteReg);
	SetWriteHandler(0xA000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xCFFF, WriteIRQ);
}

static void Reset(void) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
	FDSSoundRegReset();
	FDSSound_SC();
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper359_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = PPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}

void Mapper540_Init(CartInfo *info) {
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = PPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
