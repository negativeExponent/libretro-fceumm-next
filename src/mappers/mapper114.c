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

static uint32 M114_addr[2][8] = {
	{ 0xA001, 0xA000, 0x8000, 0xC000, 0x8001, 0xC001, 0xE000, 0xE001 }, /* 0: Aladdin, The Lion King */
	{ 0xA001, 0x8001, 0x8000, 0xC001, 0xA000, 0xC000, 0xE000, 0xE001 }  /* 1: Boogerman */
};
static uint8 M114_index[2][8] = {
	{ 0, 3, 1, 5, 6, 7, 2, 4 }, /* 0: Aladdin, The Lion King */
	{ 0, 2, 5, 3, 6, 1, 7, 4 }, /* 1: Boogerman */
};

static void M114PRG(void) {
	if (mmc3.expregs[0] & 0x80) {
        uint8 bank = mmc3.expregs[0] & 0x0F;
		if (mmc3.expregs[0] & 0x20)
			setprg32(0x8000, bank >> 1);
		else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
        int cbase = (mmc3.cmd & 0x40) << 7;
		setprg8(0x8000 ^ cbase, mmc3.regs[6]);
        setprg8(0xA000, mmc3.regs[7]);
        setprg8(0xC000 ^ cbase, ~1);
        setprg8(0xE000, ~0);
    }
}

static void M114CWRAP(uint32 A, uint8 V) {
    uint16 base = (mmc3.expregs[1] << 8) & 0x100;
	setchr1(A, base | V);
}

static DECLFW(M114ExWrite) {
	mmc3.expregs[A & 0x01] = V;
	MMC3_FixPRG();
    MMC3_FixCHR();
}

static DECLFW(M114Write) {
    A = M114_addr[iNESCart.submapper & 0x01][((A >> 12) & 0x06) | (A & 0x01)];
    if (A == 0x8000) {
        V = (V & 0xC0) | M114_index[iNESCart.submapper & 0x01][V & 0x07];
    }
    switch (A) {
    case 0x8001:
        switch (mmc3.cmd & 0x07) {
        case 6:
        case 7:
            mmc3.regs[mmc3.cmd & 0x07] = V;
            MMC3_FixPRG();
            break;
        default:
            MMC3_CMDWrite(A, V);
            break;
        }
        break;
    default:
        MMC3_Write(A, V);
        break;
    }
}

static void M114Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M114ExWrite);
	SetWriteHandler(0x8000, 0xFFFF, M114Write);
}

static void M114Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

void Mapper114_Init(CartInfo *info) {
	isRevB = 0;
	/* Use NES 2.0 submapper to identify scrambling pattern, otherwise CRC32 for Boogerman and test rom */
    if (!info->iNES2) {
        if ((info->CRC32 == 0x80eb1839) || (info->CRC32 == 0x071e4ee8)) {
            info->submapper = 1;
        }
    }

	GenMMC3_Init(info, 0, 0);
	MMC3_FixPRG = M114PRG;
	MMC3_cwrap = M114CWRAP;
	info->Power = M114Power;
	info->Reset = M114Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
