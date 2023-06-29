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

static void M012CW(uint32 A, uint8 V) {
	setchr1(A, (mmc3.expregs[(A & 0x1000) >> 12] << 8) + V);
}

static DECLFW(M012Write) {
	mmc3.expregs[0] = V & 0x01;
	mmc3.expregs[1] = (V & 0x10) >> 4;
}

static DECLFR(M012Read) {
	return mmc3.expregs[2];
}

static void M012Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	mmc3.expregs[2] = 1; /* chinese is default */
	GenMMC3Power();
	SetWriteHandler(0x4100, 0x5FFF, M012Write);
	SetReadHandler(0x4100, 0x5FFF, M012Read);
}

static void M012Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	mmc3.expregs[2] ^= 1;
	MMC3RegReset();
}

void Mapper012_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M012CW;
	isRevB = 0;

	info->Power = M012Power;
	info->Reset = M012Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
