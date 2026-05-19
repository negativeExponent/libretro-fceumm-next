/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* NES 2.0 Mapper 210 - simplified version of Mapper 19
 * Namco 175 - submapper 1 - optional wram, hard-wired mirroring
 * Namco 340 - submapper 2 - selectable H/V/0 mirroring
 */

#include "mapinc.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t wram_enable;
} m210;

static SFORMAT StateRegs[] = {
	{ m210.prg, 4, "PREG" },
	{ m210.chr, 8, "CREG" },
	{ &m210.wram_enable, 1, "WRME" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m210.prg[0] & 0x3F);
	setprg8(0xA000, m210.prg[1] & 0x3F);
	setprg8(0xC000, m210.prg[2] & 0x3F);
	setprg8(0xE000, m210.prg[3] & 0x3F);
}

static void SyncCHR(void) {
	setchr1(0x0000, m210.chr[0]);
	setchr1(0x0400, m210.chr[1]);
	setchr1(0x0800, m210.chr[2]);
	setchr1(0x0C00, m210.chr[3]);
	setchr1(0x1000, m210.chr[4]);
	setchr1(0x1400, m210.chr[5]);
	setchr1(0x1800, m210.chr[6]);
	setchr1(0x1C00, m210.chr[7]);
}

static void SyncWRAM(void) {
	/* Family Circuit '91 relies on its 2 KiB of WRAM being correctly mirrored throughout the $6000-$7FFF address range. */
	/* setprg2r_access(0x10, 0x6000, 0, TRUE, m210.wram_enable);
	setprg2r_access(0x10, 0x6800, 0, TRUE, m210.wram_enable);
	setprg2r_access(0x10, 0x7000, 0, TRUE, m210.wram_enable);
	setprg2r_access(0x10, 0x7800, 0, TRUE, m210.wram_enable); */
	setprg8r(0x10, 0x6000, 0);
}

static void SyncMirror(void) {
	if (iNESCart.submapper == 1) {
		setmirror(iNESCart.mirror);
	} else {
		switch ((m210.prg[0] >> 6) & 0x03) {
		case 0:
			setmirror(MI_0);
			break;
		case 1:
			setmirror(MI_V);
			break;
		case 2:
			setmirror(MI_H);
			break;
		case 3:
			setmirror(MI_0);
			break;
		}
	}
}

static DECLFR(ReadWRAM) {
	return WRAM[(A - 0x6000) & (WRAMSIZE - 1)];
}

static DECLFW(WriteWRAM) {
	WRAM[(A - 0x6000) & (WRAMSIZE - 1)] = V;
}

static DECLFW(WriteCHR) {
	m210.chr[(A - 0x8000) >> 11] = V;
	SyncCHR();
}

static DECLFW(WriteWRAMEnable) {
	m210.wram_enable = V & 0x01;
	SyncWRAM();
}

static DECLFW(WritePRG) {
	m210.prg[(A - 0xE000) >> 11] = V;
	SyncPRG();
	SyncMirror();
}

static void Power(void) {
	int i;
	for (i = 0; i < 4; i++) {
		m210.prg[i] = 0xFC | i;
	}
	for (i = 0; i < 8; i++) {
		m210.chr[i] = i;
	}
	m210.wram_enable = 0;

	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xDFFF, WriteWRAMEnable);
	SetWriteHandler(0xE000, 0xF7FF, WritePRG);

	if (WRAM) {
		SetReadHandler(0x6000, 0x7FFF, ReadWRAM);
		SetWriteHandler(0x6000, 0x7FFF, WriteWRAM);
		FCEU_CheatAddRAM(8, 0x6000, WRAM);
	}

	if (WRAM && !iNESCart.battery) {
		FCEU_MemoryRand(WRAM, sizeof(WRAM));
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMirror();
}

void Mapper210_Init(CartInfo *info) {
	GameStateRestore = StateRestore;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	} else {
		WRAMSIZE = 8192;
	}

	if (!WRAMSIZE && info->battery) {
		WRAMSIZE = 8192;
	}

	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}
