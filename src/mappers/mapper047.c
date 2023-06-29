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

static void M047PW(uint32 A, uint8 V) {
	setprg8(A, (mmc3.expregs[0] << 4) | (V & 0x0F));
}

static void M047CW(uint32 A, uint8 V) {
	setchr1(A, (mmc3.expregs[0] << 7) | (V & 0x7F));
}

static DECLFW(M047Write) {
    if (MMC3CanWriteToWRAM()) {
	    mmc3.expregs[0] = V;
	    MMC3_FixPRG();
	    MMC3_FixCHR();
    }
}

static void M047Reset(void) {
	MMC3RegReset();
}

static void M047Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M047Write);
}

void Mapper047_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_pwrap = M047PW;
	MMC3_cwrap = M047CW;
	info->Power = M047Power;
	info->Reset = M047Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
