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
 */

/* NES 2.0 Mapper 263 - UNL-KOF97 */

#include "mapinc.h"
#include "mmc3.h"

static uint32 unscrambleAddr(uint32 A) {
	return ((A & 0xE000) | ((A >> 12) & 1));
}

static uint8 unscrambleData(uint8 V) {
	return ((V & 0xD8) | ((V & 0x20) >> 4) | ((V & 4) << 3) | ((V & 2) >> 1) | ((V & 1) << 2));
}

static DECLFW(UNLKOF97CMDWrite) {
	MMC3_CMDWrite(unscrambleAddr(A), unscrambleData(V));
}

static DECLFW(UNLKOF97IRQWrite) {
	MMC3_IRQWrite(unscrambleAddr(A), unscrambleData(V));
}

static void UNLKOF97Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x8000, 0xA000, UNLKOF97CMDWrite);
	SetWriteHandler(0xC000, 0xF000, UNLKOF97IRQWrite);
}

void UNLKOF97_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 256, 0, 0);
	info->Power = UNLKOF97Power;
}
