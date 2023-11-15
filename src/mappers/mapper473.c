/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2024 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* NES 2.0 Mapper 473 is a variant of mapper 45 where the
 * ASIC's PRG A21/CHR A20 output (set by bit 6 of the third write to $6000)
 * selects between regularly-banked CHR-ROM (=0) and 8 KiB of unbanked CHR-RAM
 * (=1). It is used solely for the Super 8-in-1 - 98格鬥天王＋熱血 (JY-302)
 * multicart.
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg[4];

static void M473CW(uint16 A, uint16 V) {
	if (~reg[0] & 0x80) {
		setchr8(reg[3]);
	} else {
		setchr1(A, V);
	}
}

static void M473PW(uint16 A, uint16 V) {
	uint32 mask = (reg[0] & 0x20) ? (0xFF >> (0x07 - (reg[0] & 0x07))) : 0x00;
	uint32 base = (reg[0] & 0x20) ? (reg[1] | (reg[2] << 8)) : 0x3F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(M473Write) {
	reg[A & 0x03] = V;
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static DECLFW(M473WriteMMC3) {
	if (reg[0] & 0x40) {
		CartBW(A, V);
	} else {
		MMC3_Write(A, V);
	}
}

static void M473Reset(void) {
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	MMC3_Reset();
}

static void M473Power(void) {
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	MMC3_Power();
	SetWriteHandler(0x4800, 0x4FFF, M473Write);
	SetWriteHandler(0x8000, 0xFFFF, M473WriteMMC3);
}

void Mapper473_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_cwrap = M473CW;
	MMC3_pwrap = M473PW;
	info->Reset = M473Reset;
	info->Power = M473Power;
	AddExState(reg, 4, 0, "EXPR");
}
