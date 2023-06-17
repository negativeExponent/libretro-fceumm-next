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

#include "mapinc.h"
#include "mmc3.h"

static void M245CW(uint32 A, uint8 V) {
	setchr8(0);
	MMC3_FixPRG();
}

static void M245PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x3F) | ((MMC3GetCHRBank(mmc3.expregs[0]) & 0x02) << 5));
}

static void M245PPUHook(uint32 A) {
	if (A < 0x2000) {
		A >>= 10;
		A &= 3;
		if (A != mmc3.expregs[0]) {
			mmc3.expregs[0] = A;
			MMC3_FixPRG();
		}
	}
}

static void M245Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
}

void Mapper245_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	info->Power = M245Power;
	MMC3_pwrap = M245PW;
	MMC3_cwrap = M245CW;
	PPU_hook = M245PPUHook;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
