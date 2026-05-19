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

/* Mapper 64 - 	Tengen 800032 Rambo-1
 * Mapper 158 -	Tengen 800037 (Alien Syndrome Unl)
 */

#include "mapinc.h"

#define IRQ_MODE_A12 0
#define IRQ_MODE_CPU 1

static struct {
	uint8_t prg[4], chr[8];
	uint8_t cmd, mirror;
	uint8_t IRQa;
	uint8_t IRQPrescaler;
	uint8_t IRQCount;
	uint8_t IRQLatch;
	uint8_t IRQLatchExtra;
	uint8_t IRQMode;
	uint8_t IRQA12;
	uint8_t IRQFilter;
	uint8_t IRQDelay;
	uint8_t IRQReload;
} m064;

static SFORMAT StateRegs[] = {
	{ m064.prg, 4, "PREG" },
	{ m064.chr, 8, "CREG" },
	{ &m064.cmd, 1, "CMDR" },
	{ &m064.mirror, 1, "MIRR" },

	{ &m064.IRQa, 1, "IRQA" },
	{ &m064.IRQPrescaler, 1, "IQPR" },
	{ &m064.IRQCount, 1, "IRQC" },
	{ &m064.IRQLatch, 1, "IRQL" },
	{ &m064.IRQLatchExtra, 1, "IQLE" },
	{ &m064.IRQReload, 1, "IRQR" },
	{ &m064.IRQMode, 1, "IRQM" },
	{ &m064.IRQA12, 1, "IQ12" },
	{ &m064.IRQFilter, 1, "IRQF" },
	{ &m064.IRQDelay, 1, "IRQD" },

	{ 0 }
};

static void IRQClockCounter(void) {
	if (m064.IRQCount == 0) {
		m064.IRQCount = m064.IRQLatch + (m064.IRQReload ? m064.IRQLatchExtra : 0);
		if ((m064.IRQCount == 0) && m064.IRQReload && m064.IRQa) {
			m064.IRQDelay = 1;
		}
	} else {
		m064.IRQCount--;
		if ((m064.IRQCount == 0) && m064.IRQa) {
			m064.IRQDelay = 1;
		}
	}
	m064.IRQReload = FALSE;
}

/* NEWPPU Only irq timing */

static void newppu_CPUIRQHook(int a) {
	while (a--) {
		if (m064.IRQDelay) {
			m064.IRQDelay--;
			if (m064.IRQDelay == 0) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
		m064.IRQPrescaler++;
		if (!(m064.IRQPrescaler & 0x03) && (m064.IRQMode == IRQ_MODE_CPU)) {
			IRQClockCounter();
		}

		if (m064.IRQA12) {
			if (!m064.IRQFilter && (m064.IRQMode == IRQ_MODE_A12)) {
				IRQClockCounter();
			}
			m064.IRQFilter = 16;
		} else if (m064.IRQFilter) {
			m064.IRQFilter--;
		}
	}
}

static void newppu_PPUHook(uint32_t A) {
	m064.IRQA12 = (A & 0x1000) >> 12;
}

/******************/

static void CPUIRQHook(int a) {
	while (a--) {
		if (m064.IRQDelay) {
			m064.IRQDelay--;
			if (m064.IRQDelay == 0) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
		m064.IRQPrescaler++;
		if (!(m064.IRQPrescaler & 0x03) && (m064.IRQMode == IRQ_MODE_CPU)) {
			IRQClockCounter();
		}
	}
}

static void HBIRQHook(void) {
	if (((m064.IRQMode == IRQ_MODE_A12) && (scanline != 240)) /*&& (scanline != 240)*/) {
		IRQClockCounter();
	}
}

static void SyncPRG(void) {
	uint16_t pswap = (m064.cmd << 8) & 0x4000;

	setprg8(0x8000 ^ pswap, m064.prg[0]);
	setprg8(0xA000,         m064.prg[1]);
	setprg8(0xC000 ^ pswap, m064.prg[2]);
	setprg8(0xE000,         m064.prg[3]);
}

static void SyncCHR(void) {
	uint16_t cswap = (m064.cmd << 5) & 0x1000;

	if (m064.cmd & 0x20) {
		setchr1(0x0000 ^ cswap, m064.chr[0]);
		setchr1(0x0400 ^ cswap, m064.chr[6]);
		setchr1(0x0800 ^ cswap, m064.chr[1]);
		setchr1(0x0C00 ^ cswap, m064.chr[7]);
	} else {
		setchr2(0x0000 ^ cswap, (m064.chr[0] >> 1));
		setchr2(0x0800 ^ cswap, (m064.chr[1] >> 1));
	}
	setchr1(0x1000 ^ cswap, m064.chr[2]);
	setchr1(0x1400 ^ cswap, m064.chr[3]);
	setchr1(0x1800 ^ cswap, m064.chr[4]);
	setchr1(0x1C00 ^ cswap, m064.chr[5]);
}

static void SyncMirror(void) {
	if (iNESCart.mapper == 64) {
		setmirror((m064.mirror & 1) ^ 1);
	} else if (iNESCart.mapper == 158) {
		if (m064.cmd & 0x20) {
			setntamem(NTARAM + ((m064.chr[0] >> 7) << 10), TRUE, 0);
			setntamem(NTARAM + ((m064.chr[6] >> 7) << 10), TRUE, 1);
			setntamem(NTARAM + ((m064.chr[1] >> 7) << 10), TRUE, 2);
			setntamem(NTARAM + ((m064.chr[7] >> 7) << 10), TRUE, 3);
		} else {
			setntamem(NTARAM + ((m064.chr[0] >> 7) << 10), TRUE, 0);
			setntamem(NTARAM + ((m064.chr[0] >> 7) << 10), TRUE, 1);
			setntamem(NTARAM + ((m064.chr[1] >> 7) << 10), TRUE, 2);
			setntamem(NTARAM + ((m064.chr[1] >> 7) << 10), TRUE, 3);
		}
	}
}

static int ppumode = -1;

static void CheckPPUMode(void) {
	if (ppumode != newppu) {
		if (newppu) {
			MapIRQHook = newppu_CPUIRQHook;
			PPU_hook = newppu_PPUHook;
			GameHBIRQHook = NULL;
		} else {
			MapIRQHook = CPUIRQHook;
			PPU_hook = NULL;
			GameHBIRQHook = HBIRQHook;
		}
		ppumode = newppu;
	}
}

static DECLFW(WriteReg) {
	uint8_t index;

	CheckPPUMode();

	switch (A & 0xE001) {
	case 0x8000:
		m064.cmd = V;
		SyncPRG();
		SyncCHR();
		SyncMirror();
		break;
	case 0x8001:
		index = m064.cmd & 0x0F;
		switch (index) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
			m064.chr[index] = V;
			SyncCHR();
			SyncMirror();
			break;
		case 0x06:
		case 0x07:
			m064.prg[index & 0x01] = V;
			SyncPRG();
			break;
		case 0x08:
		case 0x09:
			m064.chr[index - 2] = V;
			SyncCHR();
			SyncMirror();
			break;
		case 0x0F:
			m064.prg[2] = V;
			SyncPRG();
			break;
		}
		break;
	case 0xA000:
		m064.mirror = V;
		SyncMirror();
		break;
	case 0xC000:
		m064.IRQLatch = V;
		break;
	case 0xC001:
		m064.IRQMode = (V & 0x01) ? IRQ_MODE_CPU : IRQ_MODE_A12;
		m064.IRQPrescaler = 0;
		m064.IRQCount = 0;
		m064.IRQReload = TRUE;
		m064.IRQLatchExtra = m064.IRQFilter ? 0 : 1;
		break;
	case 0xE000:
		m064.IRQa = FALSE;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xE001:
		m064.IRQa = TRUE;
		break;
	}
}

static void Power(void) {
	memset(&m064, 0, sizeof(m064));

	m064.prg[0] = 0;
	m064.prg[1] = 1;
	m064.prg[2] = ~1;
	m064.prg[3] = ~0;

	m064.chr[0] = 0;
	m064.chr[1] = 1;
	m064.chr[2] = 2;
	m064.chr[3] = 3;
	m064.chr[4] = 4;
	m064.chr[5] = 5;
	m064.chr[6] = 6;
	m064.chr[7] = 7;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	CheckPPUMode();

	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper064_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
