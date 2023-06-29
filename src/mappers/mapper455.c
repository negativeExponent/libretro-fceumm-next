/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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

static void M455PW(uint32 A, uint8 V) {
	uint8 prgAND = (mmc3.expregs[1] & 0x01) ? 0x1F : 0x0F;
	uint8 prgOR  = ((mmc3.expregs[0] >> 2) & 0x07) | ((mmc3.expregs[1] << 1) & 0x08) | ((mmc3.expregs[0] >> 2) & 0x10);
	if (mmc3.expregs[0] & 0x01) {
		if (mmc3.expregs[0] & 0x02) {
			setprg32(0x8000, prgOR >> 1);
		} else {
			setprg16(0x8000, prgOR);
			setprg16(0xC000, prgOR);
		}
	} else {
		setprg8(A, (V & prgAND) | ((prgOR << 1) & ~prgAND));
	}
}

static void M455CW(uint32 A, uint8 V) {
	uint8 chrAND = (mmc3.expregs[1] & 0x02) ? 0xFF : 0x7F;
	uint8 chrOR  = ((mmc3.expregs[0] >> 2) & 0x07) | ((mmc3.expregs[1] << 1) & 0x08) | ((mmc3.expregs[0] >> 2) & 0x10);
	setchr1(A, (V & chrAND) | ((chrOR << 4) & ~chrAND));
}

static DECLFW(M455Write) {
	if (A & 0x100) {
		mmc3.expregs[0] = V;
		mmc3.expregs[1] = A & 0xFF;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M455Reset(void) {
	mmc3.expregs[0] = 1;
	mmc3.expregs[1] = 0;
	MMC3RegReset();
}

static void M455Power(void) {
	mmc3.expregs[0] = 1;
	mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x4100, 0x5FFF, M455Write);
}

void Mapper455_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_cwrap       = M455CW;
	MMC3_pwrap       = M455PW;
	info->Power = M455Power;
	info->Reset = M455Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
