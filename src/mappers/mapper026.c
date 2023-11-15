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
 *
 *
 */

#include "mapinc.h"
#include "vrc6.h"

static void M026PW(uint32 A, uint8 V) {
    setprg8(A, V & 0x3F);
}

static void M026CW(uint32 A, uint8 V) {
    setchr1(A, V & 0xFF);
}

void Mapper026_Init(CartInfo *info) {
	VRC6_Init(info, 0x02, 0x01, 1);
    VRC6_pwrap = M026PW;
    VRC6_cwrap = M026CW;
}
