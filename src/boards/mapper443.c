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

/* NC3000M PCB */

#include "mapinc.h"
#include "mmc3.h"

static uint8 dip;

static void Mapper443_PRGWrap(uint32 A, uint8 V) {
	int prgAND = 0x0F;
	int prgOR = ((mmc3.expregs[0] << 4) & 0x20) | (mmc3.expregs[0] & 0x10);
	if (mmc3.expregs[0] & 0x04) {
		if (~A & 0x4000) {
			setprg8(A, ((((~mmc3.expregs[0] & 0x08) ? ~2 : ~0) & V) & prgAND) | (prgOR & ~prgAND));
			setprg8(A | 0x4000, ((~mmc3.expregs[0] & 0x08) ? 2 : 0) | (V & prgAND) | (prgOR & ~prgAND));
		}
	} else
		setprg8(A, (V & prgAND) | (prgOR & ~prgAND));
}

static void Mapper443_CHRWrap(uint32 A, uint8 V) {
	int chrAND = 0xFF;
	int chrOR = mmc3.expregs[0] << 8;
	setchr1(A, (V & chrAND) | (chrOR & ~chrAND));
}

static DECLFR(Mapper443_Read) {
	return (((mmc3.expregs[0] & 0x0C) == 0x08) ? dip : CartBR(A));
}

static DECLFW(Mapper443_Write) {
	mmc3.expregs[0] = A & 0xFF;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void Mapper443_Reset(void) {
	dip++;
	dip &= 15;
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void Mapper443_Power(void) {
	dip = 0;
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, Mapper443_Write);
	SetReadHandler(0x8000, 0xFFFF, Mapper443_Read);
}

void Mapper443_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 0, 0);
	cwrap = Mapper443_CHRWrap;
	pwrap = Mapper443_PRGWrap;
	info->Power = Mapper443_Power;
	info->Reset = Mapper443_Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
	AddExState(&dip, 1, 0, "DIPS");
}
