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

static void M249PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x02) {
		if (V < 0x20) {
			V = (V & 0x01) | ((V >> 3) & 0x02) | ((V >> 1) & 0x04) | ((V << 2) & 0x08) | ((V << 2) & 0x10);
		} else {
			V -= 0x20;
			V = (V & 0x03) | ((V >> 1) & 0x04) | ((V >> 4) & 0x08) | ((V >> 2) & 0x10) | ((V << 3) & 0x20) | ((V << 2) & 0xC0);
		}
	}
	setprg8(A, V);
}

static void M249CW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x2) {
		V = (V & 0x03) | ((V >> 1) & 0x04) | ((V >> 4) & 0x08) | ((V >> 2) & 0x10) | ((V << 3) & 0x20) | ((V << 2) & 0xC0);
    }
	setchr1(A, V);
}

static DECLFW(M249Write) {
	mmc3.expregs[0] = V;
	FixMMC3PRG();
	FixMMC3CHR();
}

static void M249Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5000, M249Write);
}

void Mapper249_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M249CW;
	mmc3.pwrap = M249PW;
	info->Power = M249Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
