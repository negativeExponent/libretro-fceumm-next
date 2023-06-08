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

/* NES 2.0 Mapper 404 - JY012005
 * 1998 Super HiK 8-in-1 (JY-021B)
 */

#include "mmc1.h"

static uint8 reg;

static void M404PW(uint32 A, uint8 V) {
	uint8 mask = (reg & 0x40) ? 0x07 : 0x0F;
	setprg16(A, (V & mask) | ((reg << 3) & ~mask));
}

static void M404CW(uint32 A, uint8 V) {
	setchr4(A, (V & 0x1F) | (reg << 5));
}

static DECLFW(M404Write) {
	if (!(reg & 0x80)) {
		reg = V;
		FixMMC1PRG();
		FixMMC1CHR();
	}
}

static void M404Reset(void) {
	reg = 0;
	MMC1RegReset();
}

static void M404Power(void) {
	reg = 0;
	GenMMC1Power();
	SetWriteHandler(0x6000, 0x7FFF, M404Write);
}

void Mapper404_Init(CartInfo *info) {
	GenMMC1_Init(info, MMC1B, 256, 256, 0, 0);
	info->Power = M404Power;
	info->Reset = M404Reset;
	mmc1.cwrap = M404CW;
	mmc1.pwrap = M404PW;
	AddExState(&reg, 1, 0, "BANK");
}
