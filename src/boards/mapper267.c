/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2020
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

/* NES 2.0 Mapper 267 - 8-in-1 JY-119 */

#include "mapinc.h"
#include "mmc3.h"

#define OUTER_BANK (((mmc3.expregs[0] & 0x20) >> 2) | (mmc3.expregs[0] & 0x06))

static void M267CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | (OUTER_BANK << 6));
}

static void M267PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x1F) | (OUTER_BANK << 4));
}

static DECLFW(M267Write) {
	if (!(mmc3.expregs[0] & 0x80)) {
		mmc3.expregs[0] = V;
		FixMMC3PRG();
		FixMMC3CHR();
	}
}

static void M267Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M267Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M267Write);
}

void Mapper267_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = M267CW;
	mmc3.pwrap = M267PW;
	info->Reset = M267Reset;
	info->Power = M267Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
