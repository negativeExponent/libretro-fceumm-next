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

/* 841026C and 850335C multicart circuit boards */

#include "mapinc.h"
#include "mmc3.h"

static void M441PW(uint32 A, uint8 V) {
	uint8 prgAND = (mmc3.expregs[0] & 0x08) ? 0x0F : 0x1F;
	uint8 prgOR  = (mmc3.expregs[0] << 4) & 0x30;
	if (mmc3.expregs[0] & 0x04) {
		setprg8(0x8000, (prgOR & ~prgAND) | ((mmc3.regs[6] & ~0x02) & prgAND));
		setprg8(0xA000, (prgOR & ~prgAND) | ((mmc3.regs[7] & ~0x02) & prgAND));
		setprg8(0xC000, (prgOR & ~prgAND) | ((mmc3.regs[6] |  0x02) & prgAND));
		setprg8(0xE000, (prgOR & ~prgAND) | ((mmc3.regs[7] |  0x02) & prgAND));
	} else
		setprg8(A, (prgOR & ~prgAND) | (V & prgAND));
}

static void M441CW(uint32 A, uint8 V) {
	uint16 chrAND = (mmc3.expregs[0] & 0x40) ? 0x7F : 0xFF;
	uint16 chrOR  = (mmc3.expregs[0] << 3) & 0x180;
	setchr1(A, (chrOR & ~chrAND) | (V & chrAND));
}

static DECLFW(M441Write) {
	if (~mmc3.expregs[0] & 0x80) {
		mmc3.expregs[0] = V;
	    FixMMC3PRG(mmc3.cmd);
	    FixMMC3CHR(mmc3.cmd);
    }
}

static void M441Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M441Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M441Write);
}

void Mapper441_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 0, 0);
	cwrap       = M441CW;
	pwrap       = M441PW;
	info->Power = M441Power;
	info->Reset = M441Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
