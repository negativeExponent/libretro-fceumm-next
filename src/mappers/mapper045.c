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

static void M045CW(uint32 A, uint8 V) {
	if (UNIFchrrama) {
		/* assume chr-ram, 4-in-1 Yhc-Sxx-xx variants */
		setchr8(0);
	} else {
		uint32 mask = 0xFF >> (~mmc3.expregs[2] & 0xF);
		uint32 base = ((mmc3.expregs[2] << 4) & 0xF00) | mmc3.expregs[0];
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static void M045PW(uint32 A, uint8 V) {
	uint32 mask = ~mmc3.expregs[3] & 0x3F;
	uint32 base = ((mmc3.expregs[2] << 2) & 0x300) | mmc3.expregs[1];
	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFR(M045ReadCart) {
    /* Some multicarts select between five different menus by connecting one of the higher address lines to PRG /CE.
	The menu code selects between menus by checking which of the higher address lines disables PRG-ROM when set. */
	if (((PRGsize[0] < 0x200000) && (mmc3.expregs[5] == 1) && (mmc3.expregs[1] & 0x80)) ||
	    ((PRGsize[0] < 0x200000) && (mmc3.expregs[5] == 2) && (mmc3.expregs[2] & 0x40)) ||
	    ((PRGsize[0] < 0x100000) && (mmc3.expregs[5] == 3) && (mmc3.expregs[1] & 0x40)) ||
	    ((PRGsize[0] < 0x100000) && (mmc3.expregs[5] == 4) && (mmc3.expregs[2] & 0x20))) {
		return X.DB;
	}
	return CartBR(A);
}

static DECLFW(M045WriteReg) {
	CartBW(A, V);
	if (!(mmc3.expregs[3] & 0x40)) {
		mmc3.expregs[mmc3.expregs[4]] = V;
		mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static DECLFR(M045ReadDIP) {
	uint32 addr = 1 << (mmc3.expregs[5] + 4);
	if (A & (addr | (addr - 1)))
		return X.DB | 1;
	else
		return X.DB;
}

static void M045Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	mmc3.expregs[5]++;
	mmc3.expregs[5] &= 7;
	MMC3RegReset();
}

static void M045Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = mmc3.expregs[5] = 0;
	mmc3.expregs[2] = 0x0F;
	GenMMC3Power();
    SetReadHandler(0x8000, 0xFFFF, M045ReadCart);
	SetWriteHandler(0x6000, 0x7FFF, M045WriteReg);
	SetReadHandler(0x5000, 0x5FFF, M045ReadDIP);
}

void Mapper045_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M045CW;
	MMC3_pwrap = M045PW;
	info->Reset = M045Reset;
	info->Power = M045Power;
	AddExState(mmc3.expregs, 5, 0, "EXPR");
}
