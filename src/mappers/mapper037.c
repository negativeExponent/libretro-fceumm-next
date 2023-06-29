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

#include "mapinc.h"
#include "mmc3.h"

static void M037PW(uint32 A, uint8 V) {
	uint16 base = ((mmc3.expregs[0] << 2) & 0x10) | ((mmc3.expregs[0] & 0x03) == 0x03 ? 0x08 : 0);
	uint16 mask = (mmc3.expregs[0] << 1) | 0x07;

	setprg8(A, base | (V & mask));
}

static void M037CW(uint32 A, uint8 V) {
    uint16 base = mmc3.expregs[0] << 5;
	uint16 mask = 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(M037Write) {
    if (MMC3CanWriteToWRAM()) {
	    mmc3.expregs[0] = V;
	    MMC3_FixPRG();
	    MMC3_FixCHR();
    }
}

static void M037Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M037Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M037Write);
}

void Mapper037_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_pwrap = M037PW;
	MMC3_cwrap = M037CW;
	info->Power = M037Power;
	info->Reset = M037Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
