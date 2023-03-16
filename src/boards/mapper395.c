/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* Mapper 395 - Realtec 8210
 * Super Card 12-in-1 (SPC002)
 * Super Card 13-in-1 (SPC003)
 * Super Card 14-in-1 (King006)
 * Super Card 14-in-1 (King007)
 */

#include "mapinc.h"
#include "mmc3.h"

static void M395CW(uint32 A, uint8 V) {
	uint8 mask = mmc3.expregs[1] & 0x40 ? 0x7F : 0xFF;
	setchr1(A, (V & mask) | ((mmc3.expregs[1] & 0x10) << 3) | ((mmc3.expregs[0] & 0x30) << 4) | ((mmc3.expregs[1] & 0x20) << 5));
}

static void M395PW(uint32 A, uint8 V) {
	uint8 mask = mmc3.expregs[1] & 8 ? 0x0F : 0x1F;
	setprg8(A, (V & mask) | ((mmc3.expregs[0] & 0x30) << 1) | ((mmc3.expregs[0] & 8) << 4) | ((mmc3.expregs[1] & 1) << 4));
}

static DECLFW(M395Write) {
	if (!(mmc3.expregs[1] & 0x80)) {
		mmc3.expregs[(A >> 4) & 1] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void M395Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

static void M395Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M395Write);
}

void Mapper395_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, 0, 0);
	mmc3.cwrap = M395CW;
	mmc3.pwrap = M395PW;
	info->Power = M395Power;
	info->Reset = M395Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
