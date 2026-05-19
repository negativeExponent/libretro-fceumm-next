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
#include "mmc2.h"

void (*MMC2_pwrap)(uint16_t A, uint16_t V);
void (*MMC2_cwrap)(uint16_t A, uint16_t V);

MMC2 mmc2;

static SFORMAT StateRegs[] = {
	{ mmc2.chr, 4, "CREG" },
	{ mmc2.latch, 2, "PPUL" },
	{ &mmc2.prg, 1, "PREG" },
	{ &mmc2.mirr, 1, "MIRR" },
	{ 0 }
};

void MMC2_SetPRG_default(uint16_t A, uint16_t V) {
	setprg8(A, V);
}

void MMC2_SetCHR_default(uint16_t A, uint16_t V) {
	setchr4(A, V);
}

void MMC2_SyncPRG(void) {
	MMC2_pwrap(0x8000, mmc2.prg);
	MMC2_pwrap(0xA000, ~2);
	MMC2_pwrap(0xC000, ~1);
	MMC2_pwrap(0xE000, ~0);
}

void MMC2_SyncCHR(void) {
	MMC2_cwrap(0x0000, mmc2.chr[mmc2.latch[0] | 0]);
	MMC2_cwrap(0x1000, mmc2.chr[mmc2.latch[1] | 2]);
}

void MMC2_SyncMirror(void) {
	setmirror((mmc2.mirr & 1) ^ 1);
}

DECLFW(MMC2_Write) {
	switch (A & 0xF000) {
	case 0xA000:
		mmc2.prg = V;
		MMC2_SyncPRG();
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
		mmc2.chr[(A - 0xB000) >> 12] = V;
		MMC2_SyncCHR();
		break;
	case 0xF000:
		mmc2.mirr = V;
		MMC2_SyncMirror();
		break;
	}
}

void MMC2_PPUHook(uint32_t A) {
	uint8_t bank = (A >> 12) & 0x01;
	if ((A & 0x2000) || (((A & 0xFF0) != 0xFD0) && ((A & 0xFF0) != 0xFE0))) {
		return;
	}
	mmc2.latch[bank] = (A >> 5) & 0x01;
	MMC2_SyncCHR();
}

void MMC2_Reset(void) {
	mmc2.prg = mmc2.mirr = 0;
	mmc2.latch[0] = mmc2.latch[1] = 0;
	MMC2_SyncPRG();
	MMC2_SyncCHR();
	MMC2_SyncMirror();
}

void MMC2_Power(void) {
	MMC2_Reset();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xA000, 0xFFFF, MMC2_Write);
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void MMC2_Restore(int version) {
	MMC2_SyncPRG();
	MMC2_SyncCHR();
	MMC2_SyncMirror();
}

void MMC2_Close(void) {
}

void MMC2_Init(CartInfo *info, int wram, int battery) {
	MMC2_pwrap = MMC2_SetPRG_default;
	MMC2_cwrap = MMC2_SetCHR_default;

	info->Power = MMC2_Power;
	info->Close = MMC2_Close;
	PPU_hook = MMC2_PPUHook;

	GameStateRestore = MMC2_Restore;
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

void MMC2_SetConfig(uint8_t clear) {
	PPU_hook = MMC2_PPUHook;
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0xA000, 0xFFFF, MMC2_Write);
	if (clear) {
		mmc2.prg = mmc2.mirr = 0;
		mmc2.latch[0] = mmc2.latch[1] = 0;
	}
	MMC2_SyncPRG();
	MMC2_SyncCHR();
	MMC2_SyncMirror();
}
