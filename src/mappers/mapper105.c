/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

#include "mmc1.h"

static uint32 IRQCount;
static uint32 count_target = 0x28000000;

static void M105IRQHook(int a) {
	if (mmc1.regs[1] & 0x10) {
		IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	} else {
		IRQCount += a;
		if (IRQCount >= count_target) {
            IRQCount -= count_target;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void M105CW(uint32 A, uint8 V) {
    setchr8r(0, 0);
}

static void M105PW(uint32 A, uint8 V) {
	if (mmc1.regs[1] & 0x08) {
		setprg16(A, 8 | (V & 0x7));
	} else {
		setprg32(0x8000, (mmc1.regs[1] >> 1) & 0x03);
	}
}

static void M105Power(void) {
    count_target = 0x20000000 | ((uint32)GameInfo->cspecial << 25);
	GenMMC1Power();
}

static void M105Reset(void) {
	count_target = 0x20000000 | ((uint32)GameInfo->cspecial << 25);
	MMC1RegReset();
}

void Mapper105_Init(CartInfo *info) {
	GenMMC1_Init(info, 8, 0);
	mmc1.cwrap = M105CW;
	mmc1.pwrap = M105PW;
	MapIRQHook = M105IRQHook;
	info->Power = M105Power;
	info->Reset = M105Reset;
    AddExState(&IRQCount, 4, 0, "IRQC");
}
