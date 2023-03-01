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

/* BS-110 PCB, previously called NC7000M due to a mix-up. */

#include "mapinc.h"
#include "mmc3.h"

static void Mapper391_PRGWrap(uint32 A, uint8 V) {
	int prgAND = mmc3.expregs[0] & 0x08 ? 0x0F : 0x1F;
	int prgOR = mmc3.expregs[0] << 4 & 0x30;
	if (mmc3.expregs[0] & 0x20) {
		if (~A & 0x4000) {
			setprg8(A, ((((mmc3.expregs[0] & 0x04) ? ~2 : ~0) & V) & prgAND) | (prgOR & ~prgAND));
			setprg8(A | 0x4000, ((((mmc3.expregs[0] & 0x04) ? 2 : 0) | V) & prgAND) | (prgOR & ~prgAND));
		}
	} else
		setprg8(A, (V & prgAND) | (prgOR & ~prgAND));
}

static void Mapper391_CHRWrap(uint32 A, uint8 V) {
	int chrAND = (mmc3.expregs[0] & 0x40) ? 0x7F : 0xFF;
	int chrOR = ((mmc3.expregs[0] << 3) & 0x80) | ((mmc3.expregs[1] << 8) & 0x100);
	setchr1(A, (V & chrAND) | (chrOR & ~chrAND));
}

static DECLFW(Mapper391_Write) {
	if (~mmc3.expregs[0] & 0x80) {
		mmc3.expregs[0] = V;
		mmc3.expregs[1] = ((A >> 8) & 0xFF);
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void Mapper391_Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

static void Mapper391_Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, Mapper391_Write);
}

void Mapper391_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 0, 0);
	cwrap = Mapper391_CHRWrap;
	pwrap = Mapper391_PRGWrap;
	info->Power = Mapper391_Power;
	info->Reset = Mapper391_Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
