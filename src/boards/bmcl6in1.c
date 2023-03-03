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

/* NES 2.0 Mapper 345
 * BMC-L6IN1
 * New Star 6-in-1 Game Cartridge
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_345
 */

#include "mapinc.h"
#include "mmc3.h"

static void BMCL6IN1PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x0C)
		setprg8(A, (V & 0x0F) | (mmc3.expregs[0] & 0xC0) >> 2);
	else
		setprg32(0x8000, ((mmc3.expregs[0] & 0xC0) >> 4) | (mmc3.expregs[0] & 0x03));
}

static void BMCL6IN1MW(uint8 V) {
	if (mmc3.expregs[0] & 0x20)
		setmirror(MI_0 + ((mmc3.expregs[0] & 0x10) >> 1));
	else {
		mmc3.mirroring = V;
		setmirror((V & 1) ^ 1);
	}
}

static DECLFW(BMCL6IN1Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void BMCL6IN1Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void BMCL6IN1Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, BMCL6IN1Write);
}

void BMCL6IN1_Init(CartInfo *info) {
	GenMMC3_Init(info, 512, 8, 0, 0);
	pwrap = BMCL6IN1PW;
	mwrap = BMCL6IN1MW;
	info->Power = BMCL6IN1Power;
	info->Reset = BMCL6IN1Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
