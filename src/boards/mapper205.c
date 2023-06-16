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

/* iNES Mapper 205
 * UNIF boardname BMC-JC-016-2
 */

#include "mapinc.h"
#include "mmc3.h"

static void M205PW(uint32 A, uint8 V) {
	uint8 mask = (mmc3.expregs[0] & 0x02) ? 0x0F : 0x1F;
	setprg8(A, (mmc3.expregs[0] << 4) | (V & mask));
}

static void M205CW(uint32 A, uint8 V) {
	uint8 mask = (mmc3.expregs[0] & 0x02) ? 0x7F : 0xFF;
	setchr1(A, (mmc3.expregs[0] << 7) | (V & mask));
}

static DECLFW(M205Write) {
	mmc3.expregs[0] = V & 3;
	if (V & 1) {
		mmc3.expregs[0] |= mmc3.expregs[1];
	}
	CartBW(A, V);
	FixMMC3PRG();
	FixMMC3CHR();
}

static void M205Reset(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] ^= 2; /* solder pad */
	MMC3RegReset();
}

static void M205Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M205Write);
}

void Mapper205_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = M205PW;
	mmc3.cwrap = M205CW;
	info->Power = M205Power;
	info->Reset = M205Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
