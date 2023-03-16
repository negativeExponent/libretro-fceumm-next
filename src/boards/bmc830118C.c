/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2008 CaH4e3
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

/* NES 2.0 Mapper 348
 * M-022 MMC3 based 830118C T-106 4M + 4M */

#include "mapinc.h"
#include "mmc3.h"

static void BMC830118CCW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] << 5) & 0x180) | (V & 0x7F));
}

static void BMC830118CPW(uint32 A, uint8 V) {
	if ((mmc3.expregs[0] & 0x0C) == 0x0C) {
		setprg8(0x8000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[6] & ~2) & 0x0F));
		setprg8(0xA000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[7] & ~2) & 0x0F));
		setprg8(0xC000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[6] |  2) & 0x0F));
		setprg8(0xE000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[7] |  2) & 0x0F));
	} else {
		setprg8(A, ((mmc3.expregs[0] << 2) & 0x30) | (V & 0x0F));
	}
}

static DECLFW(BMC830118CLoWrite) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void BMC830118CReset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void BMC830118CPower(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6800, 0x68FF, BMC830118CLoWrite);
}

void BMC830118C_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 128, 0, 0);
	mmc3.pwrap = BMC830118CPW;
	mmc3.cwrap = BMC830118CCW;
	info->Power = BMC830118CPower;
	info->Reset = BMC830118CReset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
