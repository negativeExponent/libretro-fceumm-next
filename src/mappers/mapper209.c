/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Mapper 035: Basically mapper 90/209/211 with WRAM */
/* Mapper 090: Single cart, extended mirroring and ROM nametables disabled */
/* Mapper 209: Single cart, extended mirroring and ROM nametables enabled */
/* Mapper 211: Duplicate of mapper 209 */

#include "mapinc.h"
#include "jyasic.h"

static uint32_t GetPRGBank(uint32_t V) {
	return (((jyasic.mode[3] << 5) & ~0x3F) | (V & 0x3F));
}

static uint32_t GetCHRBank(uint32_t V) {
	if (jyasic.mode[3] & 0x20) {
		return (((jyasic.mode[3] << 6) & 0x600) | (V & 0x1FF));
	} else {
		return (((jyasic.mode[3] << 6) & 0x600) | ((jyasic.mode[3] << 8) & 0x100) | (V & 0x0FF));
	}
}

static void SetPRG(uint16_t A, uint32_t V) {
	setprg8(A, GetPRGBank(V));
}

static void SetCHR(uint16_t A, uint32_t V) {
	setchr1(A, GetCHRBank(V));
}

static void SetWRAM(uint16_t A, uint32_t V) {
	setprg8(A, GetPRGBank(V));
}

static void SetMirror(uint16_t A, uint32_t V) {
	setntamem(CHRptr[0] + 0x400 * (GetCHRBank(V) & CHRmask1[0]), 0, A);
}

void Mapper209_Init(CartInfo *info) {
	JYASIC_Init(info, (info->mapper != 90) ? TRUE : FALSE);
	JYASIC_pwrap = SetPRG;
	JYASIC_cwrap = SetCHR;
	JYASIC_wwrap = SetWRAM;
	JYASIC_mwrap = SetMirror;
}
