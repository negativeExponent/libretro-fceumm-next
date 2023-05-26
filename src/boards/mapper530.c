/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 *
 * NES 2.0 Mapper 530 - UNL-AX5705
 * Super Bros. Pocker Mali (VRC4 mapper)
 */

#include "mapinc.h"
#include "vrc24.h"

static void M530PW(uint32 A, uint8 V) {
	setprg8(A, ((V & 0x02) << 2) | ((V & 0x08) >> 2) | (V & ~0x0A));
}

static void M530CW(uint32 A, uint32 V) {
	setchr1(A, ((V & 0x40) >> 1) | ((V & 0x20) << 1) | (V & ~0x60));
}

static DECLFW(UNLAX5705Write) {
	A |= (A & 0x08) << 9;
	VRC24Write(A, V);
}

static void M530Power(void) {
	GenVRC24Power();
	SetWriteHandler(0x8000, 0xFFFF, UNLAX5705Write);
}

void Mapper530_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC4f, 0);
	info->Power = M530Power;
	vrc24.pwrap = M530PW;
	vrc24.cwrap = M530CW;
}
