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

/* iNES Mapper 49 */
/* BMC-STREETFIGTER-GAME4IN1 */
/* added 6-24-19:
 * BMC-STREETFIGTER-GAME4IN1 - Sic. $6000 set to $41 rather than $00 on power-up.
 */

#include "mapinc.h"
#include "mmc3.h"

static void M049PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 1) {
		setprg8(A, ((mmc3.expregs[0] >> 2) & ~0x0F) | (V & 0x0F));
	} else {
		setprg32(0x8000, (mmc3.expregs[0] >> 4) & 3);
	}
}

static void M049CW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] << 1) & 0x180) | (V & 0x7F));
}

static DECLFW(M049Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M049Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1];
	MMC3RegReset();
}

static void M049Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1];
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M049Write);
}

void Mapper049_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_cwrap = M049CW;
	MMC3_pwrap = M049PW;
	info->Reset = M049Reset;
	info->Power = M049Power;
	AddExState(mmc3.expregs, 2, 0, "EXPR");

	mmc3.expregs[1] = 0;
	if (info->PRGCRC32 == 0x408EA235) {
		mmc3.expregs[1] = 0x41; /* Street Fighter II Game 4-in-1 */
    }
}
