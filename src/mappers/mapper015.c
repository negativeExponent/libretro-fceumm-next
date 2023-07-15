/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint8 bank   = latch.data & 0x3F;
	uint8 prgA13 = latch.data >> 7;

	setprg8r(0x10, 0x6000, 0);
	switch (latch.addr & 0x03) {
	case 0:
		setprg32(0x8000, bank >> 1);
		break;
	case 1:
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | 7);
		break;
	case 2:
		setprg8(0x8000, (bank << 1) | prgA13);
		setprg8(0xA000, (bank << 1) | prgA13);
		setprg8(0xC000, (bank << 1) | prgA13);
		setprg8(0xE000, (bank << 1) | prgA13);
		break;
	case 3:
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
		break;
	}

	setchr8(0);
	setmirror(((latch.data >> 6) & 1) ^ 1);
}

void Mapper015_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 1, 0);
	info->Reset = Latch_RegReset;
}
