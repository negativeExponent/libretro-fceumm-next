/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2008 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 287
 * BMC-411120-C, actually cart ID is 811120-C, sorry ;) K-3094 - another ID
 * - 4-in-1 (411120-C)
 * - 4-in-1 (811120-C,411120-C) [p4][U]
 *
 * BMC-K-3088, similar to BMC-411120-C but without jumper or dipswitch
 * - 19-in-1(K-3088)(810849-C)(Unl)
 */

/* 2023-03-02
 - use PRG size to determine variant
 - remove forced mask for outer-bank (rely on internal mask set during PRG/CHR mapping)
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reset_flag = 0;

static void BMC411120CCW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | ((mmc3.expregs[0] & 7) << 7));
}

static void BMC411120CPW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & (8 | (reset_flag && ((ROM_size * 16) <= 512) ? 4 : 0))) {
		/* 32K Mode */
		if (A == 0x8000) {
			/* bit 0-1 of register should be used as outer bank regardless of banking modes */
			setprg32(A, ((mmc3.expregs[0] >> 4) & 3) | ((mmc3.expregs[0] & 7) << 2));
			/* FCEU_printf("32K mode: bank:%02x\n", ((mmc3.expregs[0] >> 4) & 3) | ((mmc3.expregs[0] & 7) << 2)); */
		}
	} else {
		/* MMC3 Mode */
		setprg8(A, (V & 0x0F) | ((mmc3.expregs[0] & 7) << 4));
		/* FCEU_printf("MMC3: %04x:%02x\n", A, (V & 0x0F) | ((mmc3.expregs[0] & 7) << 4)); */
	}
}

static DECLFW(BMC411120CLoWrite) {
	/*	printf("Wr: A:%04x V:%02x\n", A, V); */
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = A;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void BMC411120CReset(void) {
	mmc3.expregs[0] = 0;
	reset_flag ^= 4;
	MMC3RegReset();
}

static void BMC411120CPower(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, BMC411120CLoWrite);
}

void BMC411120C_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 512, 0, 0);
	mmc3.pwrap = BMC411120CPW;
	mmc3.cwrap = BMC411120CCW;
	info->Power = BMC411120CPower;
	info->Reset = BMC411120CReset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

void BMCK3088_Init(CartInfo *info) {
	BMC411120C_Init(info);
}
	
