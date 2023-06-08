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

/* NES 2.0 Mapper 543 - 1996 無敵智カ卡 5-in-1 (CH-501) */

#include "mmc1.h"

static uint8 reg;
static uint8 bits;
static uint8 shift;

static void M543PW(uint32 A, uint8 V) {
	setprg16(A, (V & 0x0F) | (reg << 4));
}

static void M543CW(uint32 A, uint8 V) {
	setchr4(A, (V & 0x07));
}

static void M543WW(void) {
	uint32 wramBank;

	if (reg & 0x02) {
		wramBank = 0x04 | ((reg >> 1) & 0x02) | (reg & 0x01) ;
	} else {
		wramBank = ((reg << 1) & 0x02) | ((MMC1GetCHRBank(0) >> 3) & 0x01);
	}
	setprg8r(0x10, 0x6000, wramBank);
}

static DECLFW(M543Write) {
	bits |= ((V >> 3) & 0x01) << shift++;
	if (shift == 4) {
		reg = bits;
		bits = shift = 0;
		FixMMC1PRG();
		FixMMC1CHR();
	}
}

static void M543Reset(void) {
	bits = 0;
	shift = 0;
	reg = 0;
	MMC1RegReset();
}

static void M543Power(void) {
	bits = 0;
	shift = 0;
	reg = 0;
	GenMMC1Power();
	SetWriteHandler(0x5000, 0x5FFF, M543Write);
}

void Mapper543_Init(CartInfo *info) {
	/* M543 has 32K CHR RAM but only uses 8K, so its safe to set this chr to 0 */
	GenMMC1_Init(info, MMC1B, 2048, 32, 64, info->battery ? 64 : 0);
	info->Power = M543Power;
	info->Reset = M543Reset;
	mmc1.cwrap = M543CW;
	mmc1.pwrap = M543PW;
	mmc1.wwrap = M543WW;
	AddExState(&bits, 1, 0, "BITS");
	AddExState(&shift, 1, 0, "SHFT");
	AddExState(&reg, 1, 0, "REG0");
}
