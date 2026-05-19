/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 *
 */

#include "mapinc.h"
#include "mmc4.h"

void (*MMC4_pwrap)(uint16_t A, uint16_t V);
void (*MMC4_cwrap)(uint16_t A, uint16_t V);

MMC4 mmc4;

static SFORMAT StateRegs[] = {
	{ mmc4.chr, 4, "CREG" },
	{ mmc4.latch, 2, "PPUL" },
	{ &mmc4.prg, 1, "PREG" },
	{ &mmc4.mirr, 1, "MIRR" },
	{ 0 }
};

static void MMC4_SetPRG_default(uint16_t A, uint16_t V) {
	setprg16(A, V);
}

static void MMC4_SetCHR_default(uint16_t A, uint16_t V) {
	setchr4(A, V);
}

void MMC4_SyncPRG(void) {
	MMC4_pwrap(0x8000, mmc4.prg);
	MMC4_pwrap(0xC000, ~0);
}

void MMC4_SyncCHR(void) {
	MMC4_cwrap(0x0000, mmc4.chr[mmc4.latch[0] | 0]);
	MMC4_cwrap(0x1000, mmc4.chr[mmc4.latch[1] | 2]);
}

void MMC4_SyncMirror(void) {
	setmirror((mmc4.mirr & 1) ^ 1);
}

DECLFW(MMC4_Write) {
	switch (A & 0xF000) {
	case 0xA000:
		mmc4.prg = V;
		MMC4_SyncPRG();
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
		mmc4.chr[(A - 0xB000) >> 12] = V;
		MMC4_SyncCHR();
		break;
	case 0xF000:
		mmc4.mirr = V;
		MMC4_SyncMirror();
		break;
	}
}

static void MMC4PPUHook(uint32_t A) {
	uint8_t bank = (A >> 12) & 0x01;
	if ((A & 0x2000) || (((A & 0xFF0) != 0xFD0) && ((A & 0xFF0) != 0xFE0))) {
		return;
	}
	mmc4.latch[bank] = (A >> 5) & 0x01;
	MMC4_SyncCHR();
}

void MMC4_Reset(void) {
	mmc4.prg = mmc4.mirr = 0;
	mmc4.latch[0] = mmc4.latch[1] = 0;
	MMC4_SyncPRG();
	MMC4_SyncCHR();
	MMC4_SyncMirror();
}

void MMC4_Power(void) {
	MMC4_Reset();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xA000, 0xFFFF, MMC4_Write);
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void MMC4_Restore(int version) {
	MMC4_SyncPRG();
	MMC4_SyncCHR();
	MMC4_SyncMirror();
}

void MMC4_Close(void) {
}

void MMC4_Init(CartInfo *info, int wram, int battery) {
	MMC4_pwrap = MMC4_SetPRG_default;
	MMC4_cwrap = MMC4_SetCHR_default;

	info->Power = MMC4_Power;
	info->Close = MMC4_Close;
	PPU_hook = MMC4PPUHook;

	GameStateRestore = MMC4_Restore;
	AddExState(StateRegs, ~0, 0, NULL);

	if (wram) {
		WRAMSIZE = wram * 1024;
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}

void MMC4_SetConfig(uint8_t clear) {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xA000, 0xFFFF, MMC4_Write);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	PPU_hook = MMC4PPUHook;
	if (clear) {
		mmc4.prg = mmc4.mirr = 0;
		mmc4.latch[0] = mmc4.latch[1] = 0;
	}
	MMC4_SyncPRG();
	MMC4_SyncCHR();
	MMC4_SyncMirror();
}
