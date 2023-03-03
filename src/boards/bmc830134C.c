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

/* NES 2.0 Mapper 315
 * BMC-830134C
 * Used for multicarts using 820732C- and 830134C-numbered PCBs such as 4-in-1 Street Blaster 5
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_315
 */

#include "mapinc.h"
#include "mmc3.h"

static void BMC830134CCW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] << 8) & 0x100) | ((mmc3.expregs[0] << 6) & 0x80) | ((mmc3.expregs[0] << 3) & 0x40) | V);
}

static void BMC830134CPW(uint32 A, uint8 V) {
	if ((mmc3.expregs[0] & 0x06) == 0x06) {
		setprg8(0x8000, ((mmc3.expregs[0] << 3) & 0x30) | ((mmc3.regs[6] & ~2) & 0x0F));
		setprg8(0xA000, ((mmc3.expregs[0] << 3) & 0x30) | ((mmc3.regs[7] & ~2) & 0x0F));
		setprg8(0xC000, ((mmc3.expregs[0] << 3) & 0x30) | ((mmc3.regs[6] |  2) & 0x0F));
		setprg8(0xE000, ((mmc3.expregs[0] << 3) & 0x30) | ((mmc3.regs[7] |  2) & 0x0F));
	} else {
		setprg8(A, (V & 0x0F) | ((mmc3.expregs[0] & 0x06) << 3));
	}
}

static DECLFW(BMC830134CWrite) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void BMC830134CReset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void BMC830134CPower(void) {
	GenMMC3Power();
	SetWriteHandler(0x6800, 0x68FF, BMC830134CWrite);
}

void BMC830134C_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 256, 0, 0);
	pwrap = BMC830134CPW;
	cwrap = BMC830134CCW;
	info->Power = BMC830134CPower;
	info->Reset = BMC830134CReset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
