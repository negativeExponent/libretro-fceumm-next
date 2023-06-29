/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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
 */

/* S-009. UNROM plus outer bank register at $6000-$7FFF. */

#include "mapinc.h"
#include "latch.h"

static uint8 outer;

static void Sync(void) {
	setprg16(0x8000, (outer << 3) | (latch.data & 7));
	setprg16(0xC000, (outer << 3) | 7);
	setchr8(0);
	setmirror((outer >> 5) & 1);
}

static DECLFW(M434WriteOuterBank) {
	outer = V;
	Sync();
}

static void M434Reset(void) {
	outer = 0;
	Sync();
}

static void M434Power(void) {
	outer = 0;
	LatchPower();
	SetWriteHandler(0x6000, 0x7FFF, M434WriteOuterBank);
}

void Mapper434_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 1);
	info->Reset = M434Reset;
	info->Power = M434Power;
	AddExState(&outer, 2, 0, "OUTB");
}
