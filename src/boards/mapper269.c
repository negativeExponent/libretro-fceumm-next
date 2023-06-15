/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 269
 *   Games Xplosion 121-in-1
 *   15000-in-1
 *   18000-in-1
 */

#include "mapinc.h"
#include "mmc3.h"

static void M269CW(uint32 A, uint8 V) {
	uint32 mask = 0xFF >> (~mmc3.expregs[2] & 0xF);
	uint32 base = ((mmc3.expregs[2] << 4) & 0xF00) | mmc3.expregs[0];
	setchr1(A, (base & ~mask) | (V & mask));
}

static void M269PW(uint32 A, uint8 V) {
	uint32 mask = ~mmc3.expregs[3] & 0x3F;
	uint32 base = ((mmc3.expregs[2] << 2) & 0x300) | mmc3.expregs[1];
	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(M269Write5) {
	CartBW(A, V);
	if (!(mmc3.expregs[3] & 0x80)) {
		mmc3.expregs[mmc3.expregs[4]] = V;
		mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void M269Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	MMC3RegReset();
}

static void M269Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5FFF, M269Write5);
}

static uint8 unscrambleCHR(uint8 data) {
	return ((data & 0x01) << 6) | ((data & 0x02) << 3) | ((data & 0x04) << 0) | ((data & 0x08) >> 3) |
	    ((data & 0x10) >> 3) | ((data & 0x20) >> 2) | ((data & 0x40) >> 1) | ((data & 0x80) << 0);
}

void Mapper269_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.cwrap = M269CW;
	mmc3.pwrap = M269PW;
	info->Power = M269Power;
	info->Reset = M269Reset;
	AddExState(mmc3.expregs, 5, 0, "EXPR");

	if (UNIFchrrama) {
		uint32 i;
		if (VROM) {
			FCEU_free(VROM);
		}
		VROM = (uint8*)FCEU_malloc(PRGsize[0]);
		/* unscramble CHR data from PRG */
		for (i = 0; i < PRGsize[0]; i++) {
			VROM[i] = unscrambleCHR(ROM[i]);
		}
		SetupCartCHRMapping(0, VROM, PRGsize[0], 0);
	}
}
