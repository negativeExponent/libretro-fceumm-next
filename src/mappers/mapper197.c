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

static void M197PW(uint32 A, uint8 V) {
    uint8 mask = (mmc3.expregs[0] & 0x08) ? 0x0F : 0x1F;
    setprg8(A, (mmc3.expregs[0] << 4) | (V & mask));
}

static void M197FixCHR(void) {
	switch (iNESCart.submapper) {
    case 1:
        setchr2(0x0000, mmc3.regs[1] & ~0x01);
        setchr2(0x0800, mmc3.regs[1] |  0x01);
        setchr2(0x1000, mmc3.regs[4]);
        setchr2(0x1800, mmc3.regs[5]);
        break;
    case 2:
        setchr2(0x0000, mmc3.regs[0] & ~0x01);
        setchr2(0x0800, mmc3.regs[1] |  0x01);
        setchr2(0x1000, mmc3.regs[2]);
        setchr2(0x1800, mmc3.regs[5]);
        break;
    case 0:
    case 3:
    default:
		setchr2(0x0000, mmc3.regs[0] & ~0x01);
        setchr2(0x0800, mmc3.regs[0] |  0x01);
        setchr2(0x1000, mmc3.regs[2]);
        setchr2(0x1800, mmc3.regs[3]);
		break;
	}
}

static DECLFW(M197WriteWRAM) {
    if (MMC3CanWriteToWRAM()) {
        mmc3.expregs[0] = V;
        MMC3_FixPRG();
    }
}

static DECLFW(M197Write) {
    A &= 0xE001;
    switch (A) {
    case 0x8001:
        switch (mmc3.cmd & 0x07) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            mmc3.regs[mmc3.cmd & 0x07] = V;
            MMC3_FixCHR();
            break;
        default:
            MMC3_CMDWrite(A, V);
            break;
        }
        break;
    default:
        MMC3_CMDWrite(A, V);
        break;
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
    SetWriteHandler(0x8000, 0x9FFF, M197Write);
    if (iNESCart.submapper == 3) {
        SetWriteHandler(0x6000, 0x7FFF, M197WriteWRAM);
    }
}

void Mapper197_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
    info->Power = M197Power;
    info->Reset = M197Reset;
	MMC3_FixCHR = M197FixCHR;
    if (iNESCart.submapper == 3) {
        MMC3_pwrap = M197PW;
        AddExState(mmc3.expregs, 1, 0, "EXPR");
    }
}
