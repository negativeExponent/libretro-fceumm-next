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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t chrMask;
	uint8_t chrCompare;
} m195;

static writefunc writePPU2007;
extern uint32_t RefreshAddr;

static SFORMAT StateRegs[] = {
	{ &m195.chrMask, 1, "CMSK" },
	{ &m195.chrCompare, 1, "CCMP" },
	{ 0 }
};

static void SetCHR(uint16_t A, uint16_t V) {
	if ((V & m195.chrMask) == m195.chrCompare) {
		setchr1r(0x10, A, V);
	} else {
		setchr1(A, V);
	}
}

static const uint8_t chrRamLut[8] = {
    0x28, 0x00, 0x4C, 0x64, 0x46, 0x7C, 0x04, 0xFF,
};

static DECLFW(WritePPU2007) {
	if (RefreshAddr < 0x2000) {
		uint8_t reg = RefreshAddr >> 10;
		uint8_t chrBank = MMC3_GetCHRBank(reg);

		if (chrBank & 0x80) {
			if (chrBank & 0x10) {
				/* CHR-RAM disable */
				m195.chrMask = 0x00;
				m195.chrCompare = 0xFF;
			} else {
				uint8_t index = ((chrBank >> 4) & 0x04) | ((chrBank >> 2) & 0x02) | ((chrBank >> 1) & 0x01);

				m195.chrMask = (chrBank & 0x40) ? 0xFE : 0xFC;
				m195.chrCompare = chrRamLut[index];
			}
			MMC3_SyncCHR();
		}
	}
	writePPU2007(A, V);
}

static void Power(void) {
	m195.chrMask = 0xFC;
	m195.chrCompare = 0x00;
	MMC3_Power();
	setprg4r(0x10, 0x5000, 2);
	SetWriteHandler(0x5000, 0x5FFF, CartBW);
	SetReadHandler(0x5000, 0x5FFF, CartBR);

	writePPU2007 = GetWriteHandler(0x2007);
	SetWriteHandler(0x2007, 0x2007, WritePPU2007);
}

void Mapper195_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 16, info->battery);
	info->Power = Power;
	MMC3_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 4096;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
