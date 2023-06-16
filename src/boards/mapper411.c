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
 *
 */

/* Mapper 411 - A88S-1
 * 1997 Super 7-in-1 (JY-201)
 * 1997 Super 6-in-1 (JY-202)
 * 1997 Super 7-in-1 (JY-203)
 * 1997 龍珠武鬥會 7-in-1 (JY-204)
 * 1997 Super 7-in-1 (JY-205)
 * 1997 Super 7-in-1 (JY-206)
 */

#include "mapinc.h"
#include "mmc3.h"

static void M411CW(uint32 A, uint8 V) {
	uint32 mask = (mmc3.expregs[1] & 2) ? 0xFF : 0x7F;
	setchr1(A, (V & mask) | ((mmc3.expregs[1] << 5) & 0x80) | ((mmc3.expregs[0] << 4) & 0x100));
}

static void M411PW(uint32 A, uint8 V) {
	/* NROM Mode */
	if ((mmc3.expregs[0] & 0x40) && !(mmc3.expregs[0] & 0x20)) { /* $5xx0 bit 5 check required for JY-212 */
		uint32 bank = (mmc3.expregs[0] & 1) | ((mmc3.expregs[0] >> 2) & 2) | (mmc3.expregs[0] & 4) | (mmc3.expregs[1] & 8) |
		    ((mmc3.expregs[1] >> 2) & 0x10);

		/* NROM-256 */
		if (mmc3.expregs[0] & 0x02) {
			setprg32(0x8000, bank >> 1);

			/* NROM-128 */
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}

	/* MMC3 Mode */
	else {
		uint32 mask = (mmc3.expregs[1] & 2) ? 0x1F : 0x0F;
		setprg8(A, (V & mask) | ((mmc3.expregs[1] << 1) & 0x10) | ((mmc3.expregs[1] >> 1) & 0x20));
	}
}

static DECLFW(M411Write5000) {
	mmc3.expregs[A & 1] = V;
	FixMMC3PRG();
	FixMMC3CHR();
}

static void M411Power(void) {
	mmc3.expregs[0] = 0x80;
	mmc3.expregs[1] = 0x82;
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5FFF, M411Write5000);
}

void Mapper411_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M411PW;
	mmc3.cwrap = M411CW;
	info->Power = M411Power;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
