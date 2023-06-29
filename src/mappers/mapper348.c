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

static void M348CW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] << 5) & 0x180) | (V & 0x7F));
}

static void M348PW(uint32 A, uint8 V) {
	if ((mmc3.expregs[0] & 0x0C) == 0x0C) {
		setprg8(0x8000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[6] & ~2) & 0x0F));
		setprg8(0xA000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[7] & ~2) & 0x0F));
		setprg8(0xC000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[6] |  2) & 0x0F));
		setprg8(0xE000, ((mmc3.expregs[0] << 2) & 0x30) | ((mmc3.regs[7] |  2) & 0x0F));
	} else {
		setprg8(A, ((mmc3.expregs[0] << 2) & 0x30) | (V & 0x0F));
	}
}

static DECLFW(M348Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M348Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M348Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6800, 0x68FF, M348Write);
}

void Mapper348_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_pwrap = M348PW;
	MMC3_cwrap = M348CW;
	info->Power = M348Power;
	info->Reset = M348Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
