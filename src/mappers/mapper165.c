/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/*
 * This mapper is a strange MMC2+MMC3 hybrid.  Register style, PRG, mirroring,
 * ?and even IRQs? of MMC3, with the CHR swapping and CHR latch functionality of
 * MMC2.
 *
 * There is 4k CHR-RAM in addition to any CHR-ROM present.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t latch[2];
} m165;

static SFORMAT StateRegs[] = {
	{ &m165.latch, 2, "PPUL" },
	{ 0 }
};

static void SetCHR(uint16_t A, uint16_t V) {
	if (V == 0) {
		setchr4r(0x10, A, 0);
	} else {
		setchr4(A, V >> 2);
	}
}

static void SyncCHR(void) {
	SetCHR(0x0000, MMC3_GetCHRBank(m165.latch[0] ? 2 : 0));
	SetCHR(0x1000, MMC3_GetCHRBank(m165.latch[1] ? 6 : 4));
}

static void PPUHook(uint32_t A) {
	if (A & 0x2000) {
		return;
	}
	switch (A & 0x3FF0) {
	case 0x0FD0:
		m165.latch[0] = 0;
		SetCHR(0x0000, MMC3_GetCHRBank(0));
		break;
	case 0x0FE0:
		m165.latch[0] = 1;
		SetCHR(0x0000, MMC3_GetCHRBank(2));
		break;
	case 0x1FD0:
		m165.latch[1] = 0;
		SetCHR(0x1000, MMC3_GetCHRBank(4));
		break;
	case 0x1FE0:
		m165.latch[1] = 1;
		SetCHR(0x1000, MMC3_GetCHRBank(6));
		break;
	}
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_SyncCHR();
			break;
		default:
			MMC3_Write(A, V);
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	memset(&m165, 0, sizeof(m165));
	mmc3.reg[0] = 0;
	mmc3.reg[1] = 0;
	mmc3.reg[2] = 0;
	mmc3.reg[4] = 0;
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper165_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_SyncCHR = SyncCHR;
	PPU_hook = PPUHook;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 4096;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
