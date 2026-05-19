/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *  Copyright (C) 2026 negativeExponent
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
 * YOKO mapper, almost the same as 83, TODO: figure out difference
 * Mapper 83 - 30-in-1 mapper, two modes for single game carts, one mode for
 * multigame Dragon Ball Z Party
 *
 * Mortal Kombat 2 YOKO
 * N-CXX(M), XX -  PRG+CHR, 12 - 128+256, 22 - 256+256, 14 - 128+512
 *
 */

/*
 * iNES Mapper 083 also knows as Yoko
 * 256 KiB CHR-ROM => Submapper 0 (1 KiB CHR-ROM banking, no WRAM)
 * - Street Fighter II Pro/Street Blaster II Pro
 * - Street Fighter IV Pro 10/Street Blaster IV Pro 10
 * - Street Blaster V Turbo 20
 * - Street Fighter X Turbo 40
 * - Fatal Fury 2/餓狼伝説 2
 * - Fatal Fury 2'/餓狼伝説 2'
 * 512 KiB CHR-ROM => Submapper 1 (2 KiB CHR-ROM banking, no WRAM)
 * - Super Blaster VII Turbo 28
 * - World Heroes 2/快打英雄榜 2
 * - World Heroes 2 Pro//快打英雄榜 2 Pro
 * 1024 KiB CHR-ROM => Submapper 2 (1 KiB CHR-ROM banking with outer bank, 32 KiB banked WRAM)
 * - Dragon Ball Party
 *
 * NES 2.0 264 - UNL-Yoko
 * - Mortal Kombat II/V Pro
 * - Master Fighter VI'
 */

#include "mapinc.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t low[4];
	uint8_t mode;
	uint8_t outer;
	uint8_t IRQa;
	int32_t IRQCount;

	uint8_t prgMask;
	uint8_t chrMode;
	uint16_t dipMask;

	uint8_t dipsw;
} m083;

static SFORMAT StateRegs[] = {
	{ m083.prg, 4, "PREG" },
	{ m083.chr, 8, "CREG" },
	{ &m083.mode, 1, "MODE" },
	{ &m083.outer, 1, "OUTB" },
	{ &m083.IRQCount, 4 | FCEUSTATE_RLSB, "IRQC" },
	{ &m083.IRQa, 1, "IRQA" },
	{ m083.low, 4, "LOWR" },
	{ 0 }
};

static void SyncWRAM(void) {
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, (m083.outer >> 6));
	} else if (m083.mode & 0x20) {
		setprg8(0x6000, m083.prg[3]);
	} else {
		unsetcpu8(0x6000);
	}
}

static void SyncPRG(void) {
	switch ((m083.mode >> 3) & 0x03) {
	case 0:
		setprg16(0x8000, m083.outer);
		setprg16(0xC000, m083.outer | (m083.prgMask >> 1));
		break;
	case 1:
		setprg32(0x8000, m083.outer >> 1);
		break;
	case 2:
	case 3: {
		uint16_t base = (m083.outer << 1) & ~m083.prgMask;
		setprg8(0x8000, base | (m083.prg[0] & m083.prgMask));
		setprg8(0xA000, base | (m083.prg[1] & m083.prgMask));
		setprg8(0xC000, base | (m083.prg[2] & m083.prgMask));
		setprg8(0xE000, base | (~0 & m083.prgMask));
		break;
	}
	}
}

static void SyncCHR(void) {
	switch (m083.chrMode) {
	case 0:
		setchr1(0x0000, m083.chr[0]);
		setchr1(0x0400, m083.chr[1]);
		setchr1(0x0800, m083.chr[2]);
		setchr1(0x0C00, m083.chr[3]);
		setchr1(0x1000, m083.chr[4]);
		setchr1(0x1400, m083.chr[5]);
		setchr1(0x1800, m083.chr[6]);
		setchr1(0x1C00, m083.chr[7]);
		break;
	case 1:
		setchr2(0x0000, m083.chr[0]);
		setchr2(0x0800, m083.chr[1]);
		setchr2(0x1000, m083.chr[6]);
		setchr2(0x1800, m083.chr[7]);
		break;
	case 2: {
		uint16_t base = (m083.outer << 4) & 0x300;
		setchr1(0x0000, base | m083.chr[0]);
		setchr1(0x0400, base | m083.chr[1]);
		setchr1(0x0800, base | m083.chr[2]);
		setchr1(0x0C00, base | m083.chr[3]);
		setchr1(0x1000, base | m083.chr[4]);
		setchr1(0x1400, base | m083.chr[5]);
		setchr1(0x1800, base | m083.chr[6]);
		setchr1(0x1C00, base | m083.chr[7]);
		break;
	}
	}
}

static void SyncMirror(void) {
	switch (m083.mode & 0x03) {
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

static DECLFW(WriteReg) {
	if (iNESCart.mapper == 264) {
		A = ((A >> 2) & 0x3C0) | (A & 0x3F);
	}
	switch (A & 0x300) {
	case 0x000:
		m083.outer = V;
		SyncPRG();
		SyncWRAM();
		break;
	case 0x100:
		m083.mode = V;
		SyncPRG();
		SyncWRAM();
		SyncMirror();
		break;
	case 0x200:
		if (A & 0x01) {
			m083.IRQa = m083.mode & 0x80;
			m083.IRQCount &= 0xFF;
			m083.IRQCount |= V << 8;
		} else {
			m083.IRQCount &= 0xFF00;
			m083.IRQCount |= V;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 0x300:
		A &= 0x1F;
		if (A < 0x10) {
			m083.prg[A & 0x03] = V;
			SyncPRG();
			SyncWRAM();
		} else if (A < 0x18) {
			m083.chr[A & 0x07] = V;
			SyncCHR();
		}
		break;
	}
}

static DECLFR(ReadLow) {
	if (A & m083.dipMask) {
		return m083.low[A & 0x03];
	}
	return m083.dipsw;
}

static DECLFW(WriteLow) {
	m083.low[A & 0x03] = V;
}

static void Power(void) {
	memset(&m083, 0, sizeof(m083));

	m083.prg[0] = ~0x03;
	m083.prg[1] = ~0x02;
	m083.prg[2] = ~0x01;
	m083.prg[3] = ~0x00;

	m083.chr[0] = 0;
	m083.chr[1] = 1;
	m083.chr[2] = 2;
	m083.chr[3] = 3;
	m083.chr[4] = 4;
	m083.chr[5] = 5;
	m083.chr[6] = 6;
	m083.chr[7] = 7;

	m083.mode = 0x10;

	if (iNESCart.mapper == 83) {
		m083.chrMode = iNESCart.submapper;
		m083.prgMask = 0x1F;
		m083.dipMask = 0x100;
	} else if (iNESCart.mapper == 264) {
		m083.chrMode = 1;
		m083.prgMask = 0x0F;
		m083.dipMask = 0x400;
	}

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();

	SetReadHandler(0x5000, 0x5FFF, ReadLow);
	SetWriteHandler(0x5000, 0x5FFF, WriteLow);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);

	if (WRAMSIZE) {
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

static void Reset(void) {
	m083.mode = m083.outer = 0;
	m083.dipsw++;
	FCEU_printf(" disswptch = %d\n", m083.dipMask);

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

static void CPUCycle(int a) {
	if (m083.IRQa && (m083.IRQCount > 0)) {
		if (m083.mode & 0x40) {
			m083.IRQCount -= a;
		} else {
			m083.IRQCount += a;
		}
		if (m083.IRQCount <= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			m083.IRQa = 0;
		}
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

void Mapper083_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	if (!info->iNES2) {
		if (ROM.chr.size == (512 * 1024)) {
			info->submapper = 1;
		} else if (ROM.prg.size == (1024 * 1024)) {
			info->submapper = 2;
		} else if (ROM.prg.size == (512 * 1024)) {
			info->submapper = 3;
		}
		if (info->submapper) FCEU_printf(" Submapper (overrode) : %d\n", info->submapper);
	}

	WRAMSIZE = (info->submapper == 2) ? 32768 : 0;
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
}

void Mapper264_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	MapIRQHook = CPUCycle;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
