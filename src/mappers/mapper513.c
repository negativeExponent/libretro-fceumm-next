/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

/* NES 2.0 Mapper 513 - Sachen UNL-SA-9602B */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t ppuchrbus;
} m513;

static SFORMAT StateRegs[] = {
	{ &m513.ppuchrbus, 1, "PPUC" },
	{ 0 }
};

static void SyncPRG(void) {
	/* FIXME: fetch outer from active chr read bank */
	setprg8(0x8000, (MMC3_GetPRGBank(0) & 0x3F) | (MMC3_GetCHRBank(m513.ppuchrbus) & 0xC0));
	setprg8(0xA000, (MMC3_GetPRGBank(1) & 0x3F) | (MMC3_GetCHRBank(m513.ppuchrbus) & 0xC0));
	setprg8(0xC000, (MMC3_GetPRGBank(2) & 0x3F));
	setprg8(0xE000, (MMC3_GetPRGBank(3) & 0x3F));
}


static void PPUIRQHook(uint32_t A) {
	if ((A & 0x3000) != 0x2000) {
		m513.ppuchrbus = A >> 10;
	}
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		mmc3.reg[mmc3.cmd & 0x07] = V;
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			MMC3_SyncPRG();
			MMC3_SyncCHR();
			break;
		default:
			MMC3_SyncPRG();
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper513_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_SyncPRG = SyncPRG;
	mmc3.opts |= 2;
	info->SaveGame[0] = CHRRAM;
	info->SaveGameLen[0] = info->CHRRamSaveSize;
	info->Power = Power;
	PPU_hook = PPUIRQHook;
	AddExState(StateRegs, ~0, 0, NULL);
}
