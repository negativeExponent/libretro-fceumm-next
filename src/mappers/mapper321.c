/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2024 negativeExponent
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

/* NC3000M PCB */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg;

static SFORMAT StateRegs[] = {
	{ &reg, 1, "REGS" },
	{ 0 }
};

static void M321PW(uint16 A, uint16 V) {
	if (reg & 0x08) { /* NROM */
		setprg32(0x8000, (reg & 0x04) | ((reg >> 4) & 0x03));
	} else { /*  MMC3 */
		setprg8(A, ((reg << 2) & ~0x0F) | (V & 0x0F));
	}
}

static void M321CW(uint16 A, uint16 V) {
	setchr1(A, ((reg << 5) & ~0x7F) | (V & 0x7F));
}

static DECLFW(M321Write) {
    if (MMC3_WramIsWritable()) {
        CartBW(A, V);
	    reg = V & 0xFF;
	    MMC3_FixPRG();
	    MMC3_FixCHR();
    }
}

static void M321Reset(void) {
	reg = 0;
	MMC3_Reset();
}

static void M321Power(void) {
	reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M321Write);
}

void Mapper321_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = M321CW;
	MMC3_pwrap = M321PW;
	info->Power = M321Power;
	info->Reset = M321Reset;
    AddExState(StateRegs, ~0, 0, NULL);
}
