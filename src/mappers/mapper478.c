/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025 negativeExponent
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

 /* NES 2.0 Mapper 478 */
 /* 7-in-1 (AB701) (Unl) */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg[2];

static SFORMAT StateRegs[] = {
	{ &reg[0], 1, "REGS" },
    { &reg[1], 1, "MODE" },
	{ 0 }
};

static void M478PW(uint16 A, uint16 V) {
    uint16 base = (iNESCart.submapper == 1) ? (reg[0] << 3) : (reg[0] << 2);
    uint16 mask = (iNESCart.submapper == 1) ? 0x0F : (((reg[0] & 0x0C) == 0x0C) ? 0x03 : 0x0F);

    setprg8(A, (base & ~mask) | (V & mask));
}

static void M478CW(uint16 A, uint16 V) {
	uint16 base = (iNESCart.submapper == 1) ? (reg[0] << 6) : (reg[0] << 5);
    uint16 mask = (iNESCart.submapper == 1) ? 0x7F : (((reg[0] & 0x0C) == 0x0C) ? 0x1F : 0x7F);

    setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(M478Write) {
	if (MMC3_WramIsWritable()) {
        if (reg[1] & 0x60) {
		    reg[0] = A & 0xFF;
            reg[1] = V;
		    MMC3_SyncPRG();
		    MMC3_SyncCHR();
        }
	}
}

static DECLFW(M478WriteMMC3) {
    if (reg[1] & 0x80) {
        MMC3_Write(A, V);
    } else {
        /* Mickey Mouse */
        mmc3.reg[0] = (mmc3.reg[0] & ~0x18) | ((V << 3) & 0x18);
        mmc3.reg[1] = (mmc3.reg[1] & ~0x18) | ((V << 3) & 0x18);
        mmc3.reg[2] = (mmc3.reg[2] & ~0x18) | ((V << 3) & 0x18);
        mmc3.reg[3] = (mmc3.reg[3] & ~0x18) | ((V << 3) & 0x18);
        mmc3.reg[4] = (mmc3.reg[4] & ~0x18) | ((V << 3) & 0x18);
        mmc3.reg[5] = (mmc3.reg[5] & ~0x18) | ((V << 3) & 0x18);
		MMC3_SyncCHR();
    }
}

static void M478Reset(void) {
	reg[0] = 0;
    reg[1] = 0xF0;
	MMC3_Reset();
}

static void M478Power(void) {
	reg[0] = 0;
    reg[1] = 0xF0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M478Write);
    SetWriteHandler(0x8000, 0xFFFF, M478WriteMMC3);
}

void Mapper478_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = M478CW;
	MMC3_pwrap = M478PW;
	info->Reset = M478Reset;
	info->Power = M478Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
