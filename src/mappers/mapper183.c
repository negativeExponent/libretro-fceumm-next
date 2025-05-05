/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 * iNES mapper 183
 * shui guan pipe (Gimmick Bootleg - VRC4 mapper)
 */

#include "mapinc.h"
#include "vrc24.h"

static uint8 prg[4];

static SFORMAT StateRegs[] = {
	{ prg, 4, "PRG" },
	{ 0 }
};

static void M183PRG(void) {
	setprg8(0x6000, prg[0]);
	setprg8(0x8000, prg[1]);
	setprg8(0xA000, prg[2]);
	setprg8(0xC000, prg[3]);
	setprg8(0xE000, ~0);
}

static DECLFW(M183Write6800) {
	prg[0] = A & 0x3F;
	VRC24_SyncPRG();
}

static DECLFW(M183Write8800) {
	prg[1] = V & 0x3F;
	VRC24_SyncPRG();
}

static DECLFW(M183WriteA800) {
	prg[2] = V & 0x3F;
	VRC24_SyncPRG();
}

static DECLFW(M183WriteA000) {
	prg[3] = V & 0x3F;
	VRC24_SyncPRG();
}

static void M183Power(void) {
	prg[0] = 0;
	prg[1] = 0;
	prg[2] = 1;
	prg[3] = ~1;
	VRC24_Power();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6800, 0x6FFF, M183Write6800);
	SetWriteHandler(0x8800, 0x8FFF, M183Write8800);
	SetWriteHandler(0xA800, 0xAFFF, M183WriteA800);
	SetWriteHandler(0xA000, 0xA7FF, M183WriteA000);
}

void Mapper183_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 0, 1);
	info->Power = M183Power;
	VRC24_SyncPRG = M183PRG;
	AddExState(StateRegs, ~0, 0, NULL);
}
