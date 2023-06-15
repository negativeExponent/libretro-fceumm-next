/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * A98402 board, A9711, A9746 similar
 * King of Fighters 96, The (Unl), Street Fighter Zero 2 (Unl)
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static void M187CW(uint32 A, uint8 V) {
	if ((A & 0x1000) == ((mmc3.cmd & 0x80) << 5))
		setchr1(A, V | 0x100);
	else
		setchr1(A, V);
}

static void M187PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x80) {
		uint8 bank = (mmc3.expregs[0] >> 1) & 0x0F;
		if (mmc3.expregs[0] & 0x20) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else
		setprg8(A, V & 0x3F);
}

static DECLFW(M187WriteLo) {
	if (!(A & 1)) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
	}
}

static DECLFR(M187Read) {
	return X.DB | 0x80;
}

static void M187Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetReadHandler(0x5000, 0x5FFF, M187Read);
	SetWriteHandler(0x5000, 0x5FFF, M187WriteLo);
}

void Mapper187_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M187PW;
	mmc3.cwrap = M187CW;
	info->Power = M187Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
