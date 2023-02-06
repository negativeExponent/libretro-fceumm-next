/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
#include "latch.h"

extern uint32 ROM_size;

static void Sync(void) {
	setchr8(0);
	setprg16(0xc000, 0x7);
	if (latch.data) {
		if (latch.data & 0x10)
			setprg16(0x8000, (latch.data & 7));
		else
			setprg16(0x8000, (latch.data & 7) | 8);
	} else
		setprg16(0x8000, 7 + (ROM_size >> 4));
}

static DECLFR(ExtDev) {
	return(3);
}

static void M118Power(void) {
	LatchPower();
	SetReadHandler(0x6000, 0x7FFF, ExtDev);
}

void Mapper188_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = M118Power;
}
