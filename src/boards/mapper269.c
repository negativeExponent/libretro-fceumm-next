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

static uint8 *CHRROM;
static uint32 CHRROMSIZE;

static void M269CW(uint32 A, uint8 V) {
	uint16 NV = V;
	if (mmc3.expregs[2] & 8)
		NV &= (1 << ((mmc3.expregs[2] & 7) + 1)) - 1;
	NV |= mmc3.expregs[0] | ((mmc3.expregs[2] & 0xF0) << 4);
	setchr1(A, NV);
}

static void M269PW(uint32 A, uint8 V) {
	uint16 MV = V & ((mmc3.expregs[3] & 0x3F) ^ 0x3F);
	MV |= mmc3.expregs[1];
	MV |= ((mmc3.expregs[3] & 0x40) << 2);
	setprg8(A, MV);
}

static DECLFW(M269Write5) {
	if (!(mmc3.expregs[3] & 0x80)) {
		mmc3.expregs[mmc3.expregs[4]] = V;
		mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void M269Close(void) {
	GenMMC3Close();
	if (CHRROM)
		FCEU_free(CHRROM);
	CHRROM = NULL;
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
	uint32 i;
	GenMMC3_Init(info, 512, 0, 8, 0);
	cwrap = M269CW;
	pwrap = M269PW;
	info->Power = M269Power;
	info->Reset = M269Reset;
	info->Close = M269Close;
	AddExState(mmc3.expregs, 5, 0, "EXPR");

	CHRROMSIZE = PRGsize[0];
	CHRROM = (uint8 *)FCEU_gmalloc(CHRROMSIZE);
	/* unscramble CHR data from PRG */
	for (i = 0; i < CHRROMSIZE; i++)
		CHRROM[i] = unscrambleCHR(PRGptr[0][i]);
	SetupCartCHRMapping(0, CHRROM, CHRROMSIZE, 0);
	AddExState(CHRROM, CHRROMSIZE, 0, "_CHR");
}
