/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mapinc.h"
#include "jyasic.h"

static uint32 GetPRGBank(uint32 V) {
	return ((jyasic.mode[3] << 5) | (V & 0x1F));
}

static uint32 GetCHRBank(uint32 V) {
	return ((jyasic.mode[3] << 8) | (V & 0xFF));
}

static void M281PW(uint16 A, uint32 V) {
	setprg8(A, GetPRGBank(V));
}

static void M281CW(uint16 A, uint32 V) {
	setchr1(A, GetCHRBank(V));
}

static void M281WW(uint16 A, uint32 V) {
	setprg8(A, GetPRGBank(V));
}

static void M281MW(uint16 A, uint32 V) {
	setntamem(CHRptr[0] + 0x400 * (GetCHRBank(V) & CHRmask1[0]), 0, A);
}

void Mapper281_Init(CartInfo *info) {
	/* Multicart */
	JYASIC_Init(info, TRUE);
    JYASIC_pwrap = M281PW;
	JYASIC_cwrap = M281CW;
	JYASIC_wwrap = M281WW;
	JYASIC_mwrap = M281MW;
}
