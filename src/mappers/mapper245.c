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

static uint8 reg;

static void M245PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x3F) | ((MMC3_GetCHRBank(reg) & 0x02) << 5));
}

static void M245CW(uint32 A, uint8 V) {
    setchr8(0);
}

static DECLFW(M245Write) {
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
            mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_FixPRG();
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

static void M245PPUHook(uint32 A) {
	if (A < 0x2000) {
		A >>= 10;
		A &= 3;
		if (A != reg) {
			reg = A;
			MMC3_FixPRG();
		}
	}
}

static void M245Power(void) {
	reg = 0;
	MMC3_Power();
	SetWriteHandler(0x8000, 0x9FFF, M245Write);
}

void Mapper245_Init(CartInfo *info) {
	MMC3_Init(info, 8, info->battery);
	info->Power = M245Power;
	MMC3_pwrap = M245PW;
	MMC3_cwrap = M245CW;
	PPU_hook = M245PPUHook;
	AddExState(&reg, 1, 0, "EXPR");
}
