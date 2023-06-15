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

static DECLFR(M254ReadWRAM) {
	if (mmc3.expregs[0]) {
		return X.DB ^ mmc3.expregs[0];
	}
    return CartBR(A);
}

static DECLFW(M254WriteWRAM) {
	if (V & 0x01) {
		mmc3.expregs[0] = 0;
	}
	CartBW(A, V);
}

static void M254Reset(void) {
	mmc3.expregs[0] = 0x80;
	FixMMC3CHR(mmc3.cmd);
	FixMMC3PRG(mmc3.cmd);
}

static void M254Power(void) {
    mmc3.expregs[0] = 0x80;
	GenMMC3Power();
	SetReadHandler(0x6000, 0x7FFF, M254ReadWRAM);
    SetWriteHandler(0x6000, 0x7FFF, M254WriteWRAM);
}

void Mapper254_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	info->Power = M254Power;
	info->Reset = M254Reset;
	AddExState(&mmc3.expregs[0], 1, 0, "EXPR");
}
