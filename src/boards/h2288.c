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
 */

/* iNES Mapper 123 - UNL-H2288 */

#include "mapinc.h"
#include "mmc3.h"

extern uint8 m114_perm[8];

static void H2288PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x40) {
		uint8 bank = (mmc3.expregs[0] & 5) | ((mmc3.expregs[0] & 8) >> 2) | ((mmc3.expregs[0] & 0x20) >> 2);
		if (mmc3.expregs[0] & 2)
			setprg32(0x8000, bank >> 1);
		else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(A, V & 0x3F);
	}
}

static DECLFW(H2288WriteHi) {
	if (!(A & 1)) {
		V = (V & 0xC0) | (m114_perm[V & 7]);
	}
	MMC3_CMDWrite(A, V);
}

static DECLFW(H2288WriteLo) {
	if (A & 0x800) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
	}
}

static void H2288Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5FFF, H2288WriteLo);
	SetWriteHandler(0x8000, 0x9FFF, H2288WriteHi);
}

void UNLH2288_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 0, 0);
	mmc3.pwrap = H2288PW;
	info->Power = H2288Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
