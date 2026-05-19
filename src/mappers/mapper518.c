/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

/*
 * NES 2.0 Mapper 518 is used for several games and educational computer
 * cartridges from Subor:
 *
 * ** 小霸王 Subor 999
 * ** 小霸王 Subor V
 * ** 跳舞天使: 動感 2000 (also known as Dance 2000 12-in-1)
 * Its UNIF board name is UNL-DANCE2000.
 */

#include "mapinc.h"

static struct {
	uint8_t prg, mode;
	uint16_t lastnt;
} m518;

static SFORMAT StateRegs[] = {
	{ &m518.prg, 1, "REGS" },
	{ &m518.mode, 1, "MODE" },
	{ &m518.lastnt, 2 | FCEUSTATE_RLSB, "LSNT" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, (WRAMSIZE > 8192) ? (128/8) : 0);
	if (m518.prg & 0x80) {
		if (m518.mode & 4) {
			setprg32r(0x10, 0x8000, m518.prg & 3);
		} else {
			setprg16r(0x10, 0x8000, m518.prg & 0x07);
			setprg16(0xC000, 0);
		}
	} else {
		if (m518.mode & 4) {
			setprg32(0x8000, m518.prg);
		} else {
			setprg16(0x8000, m518.prg);
			setprg16(0xC000, 0);
		}
	}
}

static void SyncCHR(void) {
	setchr4(0x0000, m518.lastnt);
	setchr4(0x1000, 1);
}

static void SyncMirror(void) {
	setmirror((m518.mode ^ 1) & 1);
}

static DECLFW(Write5000) {
	switch (A) {
	case 0x5000:
		m518.prg = V;
		SyncPRG();
		break;
	case 0x5200:
		m518.mode = V;
		SyncPRG();
		SyncMirror();
		break;
	}
}

static void PPUIRQHook(uint32_t A) {
	if (m518.mode & 2) {
		if ((A & 0x3000) == 0x2000) {
			uint32_t curnt = A & 0x800;
			if (curnt != m518.lastnt) {
				setchr4(0x0000, curnt >> 11);
				m518.lastnt = curnt;
			}
		}
	} else {
		m518.lastnt = 0;
		setchr4(0x0000, 0);
	}
}

static void Power(void) {
	memset(&m518, 0, sizeof(m518));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x5000, 0x5FFF, Write5000);
	FCEU_CheatAddRAM((WRAMSIZE & 0x1FFF) >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper518_Init(CartInfo *info) {
	info->Power = Power;
	PPU_hook = PPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = (info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192);
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
