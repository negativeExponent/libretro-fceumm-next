/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
#include "h3001.h"

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

void Mapper065_Init(CartInfo *info) {
	H3001_Init(info);
	H3001_pwrap = SetPRG;
	H3001_cwrap = SetCHR;
}
