/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
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
 */

/* NES 2.0 mapper 339 is used for a 21-in-1 multicart.
 * Its UNIF board name is BMC-K-3006. 
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_339
 */

#include "mapinc.h"
#include "mmc3.h"

static void M339CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | (mmc3.expregs[0] & 0x18) << 4);
}

static void M339PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x20) {				/* MMC3 mode */
		setprg8(A, (V & 0x0F) | (mmc3.expregs[0] & 0x18) << 1);
	} else {
		if ((mmc3.expregs[0] & 0x07) == 0x06) {	/* NROM-256 */
			setprg32(0x8000, (mmc3.expregs[0] >> 1) & 0x0F);
		} else {							/* NROM-128 */
			setprg16(0x8000, mmc3.expregs[0] & 0x1F);
			setprg16(0xC000, mmc3.expregs[0] & 0x1F);
		}
	}
}

static DECLFW(M339Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = A & 0x3F;
		FixMMC3PRG();
		FixMMC3CHR();
	}
}

static void M339Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M339Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M339Write);
}

void Mapper339_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = M339PW;
	mmc3.cwrap = M339CW;
	info->Power = M339Power;
	info->Reset = M339Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
