/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 344
 * BMC-GN-26
 * Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 (3-in-1,6-in-1,Unl)
 */

#include "mapinc.h"
#include "mmc3.h"

static void BMCGN26CW(uint32 A, uint8 V) {
	int chrAND = (mmc3.expregs[0] & 0x02) ? 0x7F : 0xFF;
	int chrOR = (mmc3.expregs[0] & 3) << 7;
	setchr1(A, chrOR | (V & chrAND));
}

static void BMCGN26PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 4) {
		setprg32(0x8000, (mmc3.expregs[0] << 2) | ((mmc3.regs[6] & 0x0F) >> 2));
	} else {
		setprg8(A, (mmc3.expregs[0] << 4) | (V & 0x0F));
	}
}

static DECLFW(BMCGN26Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = A & 0xFF;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void BMCGN26Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void BMCGN26Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, BMCGN26Write);
}

void BMCGN26_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 256, 0, 0);
	pwrap = BMCGN26PW;
	cwrap = BMCGN26CW;
	info->Power = BMCGN26Power;
	info->Reset = BMCGN26Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
