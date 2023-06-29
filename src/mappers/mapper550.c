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

/* NES 2.0 Mapper 550 - 7-in-1 1993 Chess Series (JY-015) */

#include "mmc1.h"

static uint8 latch;
static uint8 reg;

static void M550PW(uint32 A, uint8 V) {
	if ((reg & 0x06) == 0x06) {
		setprg16(A, (V & 0x07) | (reg << 2));
	} else {
		setprg32(0x8000, (latch >> 4) | (reg << 1));
	}
}

static void M550CW(uint32 A, uint8 V) {
	if ((reg & 0x06) == 0x06) {
		setchr4(A, (V & 0x07) | ((reg << 2) & 0x18));
	} else {
		setchr8((latch & 0x03) | ((reg << 1) & 0x0C));
	}
}

static DECLFW(M550Write7) {
	if (!(reg & 0x08)) {
		reg = A & 0x0F;
		FixMMC1PRG();
		FixMMC1CHR();
	}
}

static DECLFW(M550Write8) {
	latch = V;
	if ((reg & 0x06) == 0x06) {
		MMC1Write(A, V);
	}
	FixMMC1PRG();
	FixMMC1CHR();
}

static void M550Reset(void) {
	latch = 0;
	reg = 0;
	MMC1RegReset();
}

static void M550Power(void) {
	latch = 0;
	reg = 0;
	GenMMC1Power();
	SetWriteHandler(0x7000, 0x7FFF, M550Write7);
	SetWriteHandler(0x8000, 0xFFFF, M550Write8);
}

void Mapper550_Init(CartInfo *info) {
	GenMMC1_Init(info, 8, 0);
	info->Power = M550Power;
	info->Reset = M550Reset;
	mmc1.cwrap = M550CW;
	mmc1.pwrap = M550PW;
	AddExState(&latch, 1, 0, "LATC");
	AddExState(&reg, 1, 0, "REG0");
}
