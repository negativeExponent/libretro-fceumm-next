/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 * iNES Mapper 298 - UNL-TF1201
 * Lethal Weapon (VRC4 mapper)
 */

#include "mapinc.h"
#include "vrc24.h"

static DECLFW(UNLTF1201IRQWrite) {
	/* NOTE: acknowledge IRQ but do not move A control bit to E control bit.
	 * So, just make E bit the same with A bit coz im lazy to modify IRQ code for now. */
	if ((A & 0x03) == 0x01) {
		V |= ((V >> 1) & 0x01);
	}
	VRC24Write(A, V);
}

static void UNLTF1201Power(void) {
	GenVRC24Power();
	SetWriteHandler(0xF000, 0xFFFF, UNLTF1201IRQWrite);
}

void UNLTF1201_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC4b, 0);
	info->Power = UNLTF1201Power;
}
