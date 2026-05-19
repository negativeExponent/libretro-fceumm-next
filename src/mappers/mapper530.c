/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/*
 * NES 2.0 Mapper 530 is used for Super Mario Bros. Pocker Mali (sic), a bootleg
 * version of Bandai's Crayon Shin-chan: Ora to Poi Poi. It's a VRC4 clone
 * (A0/A1, VRC4f) with the PRG and CHR address lines scrambled: PRG A14 is
 * swapped with PRG A16, and CHR A15 is swapped with CHR A16. Its UNIF board
 * name is UNL-AX5705.
 *
 * Super Bros. Pocker Mali (VRC4 mapper)
 */

#include "mapinc.h"
#include "vrc24.h"

static DECLFW(WriteVRC4) {
	A = (A & ~0x1000) | ((A << 9) & 0x1000);
	switch (A & 0xF000) {
	case 0x8000:
	case 0xA000:
		V = ((V << 2) & 0x08) | ((V >> 2) & 0x02) | (V & ~0x0A);
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
		if (A & 0x0001) {
			V = ((V >> 1) & 0x02) | ((V << 1) & 0x04) | (V & ~0x06);
		}
		break;
	}
	VRC24_Write(A, V);
}

static void Power(void) {
	VRC24_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteVRC4);
}

void Mapper530_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, 0, 1);
	info->Power = Power;
}
