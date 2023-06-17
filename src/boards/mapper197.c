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

static void M197CW(uint32 A, uint8 V) {
	switch (iNESCart.submapper) {
    case 1:
        if (A == 0x0800)
            setchr4(0x0000, V >> 1);
        else if (A == 0x1800)
            setchr2(0x1000, V);
        else if (A == 0x1C00)
            setchr2(0x1800, V);
        break;
    case 2:
        if (A == 0x0000)
            setchr2(0x0000, V);
        else if (A == 0x0C00)
            setchr2(0x0800, V);
        else if (A == 0x1000)
            setchr2(0x1000, V);
        else if (A == 0x1C00)
            setchr2(0x1800, V);
        break;
    case 3:
        if (A == 0x0000)
			setchr4(0x0000, (((mmc3.expregs[0] << 7) & 0x100) | V) >> 1);
		else if (A == 0x1000)
			setchr2(0x1000, ((mmc3.expregs[0] << 7) & 0x100) | V);
		else if (A == 0x1400)
			setchr2(0x1800, ((mmc3.expregs[0] << 7) & 0x100) | V);
		break;
    case 0:
    default:
		if (A == 0x0000)
			setchr4(0x0000, V >> 1);
		else if (A == 0x1000)
			setchr2(0x1000, V);
		else if (A == 0x1400)
			setchr2(0x1800, V);
		break;
	}
}

static void M197PW(uint32 A, uint8 V) {
    uint8 mask = (mmc3.expregs[0] & 0x08) ? 0x0F : 0x1F;
    setprg8(A, (mmc3.expregs[0] << 4) | (V & mask));
}

static DECLFW(M197WriteWRAM) {
    if (MMC3CanWriteToWRAM()) {
        mmc3.expregs[0] = V;
        MMC3_FixCHR();
        MMC3_FixPRG();
    }
}

static void M197Reset(void) {
    mmc3.expregs[0] = 0;
    MMC3_FixCHR();
    MMC3_FixPRG();
}

static void M197Power(void) {
    mmc3.expregs[0] = 0;
    GenMMC3Power();
    if (iNESCart.submapper == 3) {
        SetWriteHandler(0x6000, 0x7FFF, M197WriteWRAM);
    }
}

void Mapper197_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
    info->Power = M197Power;
    info->Reset = M197Reset;
	MMC3_cwrap = M197CW;
    if (iNESCart.submapper == 3) {
        MMC3_pwrap = M197PW;
        AddExState(mmc3.expregs, 1, 0, "EXPR");
    }

}
