/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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

/* DS-9-27. Absolutely insane PCB that overlays 8 KiB of WRAM into a selectable position between $8000 and $E000. */
/* DS-9-16. Submapper 1 uses latch address instead latch data for banking modes */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	if (iNESCart.submapper == 1) {
		switch (latch.addr & 0xF000) {
		case 0xA000:
			setprg16(0x8000, latch.addr >> 1);
			setprg8(0xC000, 0);
			setprg8r(0x10, 0x8000 | ((latch.addr << 4) & 0x6000), 0);
			break;
		case 0xC000:
			setprg16(0x8000, (latch.addr >> 1) | 0);
			setprg16(0xC000, (latch.addr >> 1) | 1);
			setprg8r(0x10, 0x8000 | ((latch.addr << 4) & 0x6000), 0);
			break;
		case 0xD000:
			setprg8(0x8000, latch.addr);
			setprg8(0xA000, latch.addr);
			setprg8(0xC000, latch.addr);
			setprg8(0xE000, latch.addr);
			setprg8r(0x10, 0x8000 | ((latch.addr << 4) & 0x2000), 0);
			setprg8r(0x10, 0xC000 | ((latch.addr << 4) & 0x2000), 0);
			break;
		case 0xE000:
			setprg16(0x8000, latch.addr >> 1);
			setprg16(0xC000, (latch.addr & 0x100) ? ((latch.addr >> 1) | 0x07) : 0);
			setprg8r(0x10, 0x8000 | ((latch.addr << 4) & 0x6000), 0);
			break;
		default:
			setprg16(0x8000, latch.addr >> 1);
			setprg16(0xC000, 0);
			break;
		}
		setchr8(0);
		setmirror(((latch.addr >> 11) & 1) ^ 1);
	} else {
		if (latch.data & 0x02) {
			setprg8(0x8000, latch.addr >> 1);
			setprg8(0xA000, latch.addr >> 1);
			setprg8(0xC000, latch.addr >> 1);
			setprg8(0xE000, latch.addr >> 1);
		} else if (latch.data & 0x08) {
			setprg8(0x8000, (latch.addr >> 1 & ~0x01) | 0);
			setprg8(0xA000, (latch.addr >> 1 & ~0x01) | 1);
			setprg8(0xC000, (latch.addr >> 1 & ~0x01) | 2);
			setprg8(0xE000, (latch.addr >> 1 & ~0x01) | 3 |
				(latch.data & 0x04) |
				((latch.data & 0x04) && (latch.data & 0x40) ? 0x08 : 0x00));
		} else {
			setprg16(0x8000, latch.addr >> 2);
			setprg16(0xC000, 0);
		}

		setchr8(0);
		setmirror((latch.data & 0x01) ^ 0x01);

		setprg8r(0x10, 0x8000 | ((latch.data << 9) & 0x6000), 0);
		if (latch.data & 0x02) {
			setprg8r(0x10, 0x8000 | ((latch.data << 9) & 0x2000), 0);
		}
	}
}

static DECLFW(M452Write) {
	if ((&Page[(A & 0xE000) >> 11][A & 0xE000]) == WRAM) {
		CartBW(A, V);
	} else {
		Latch_Write(A, V);
	}
}

static void M452Power(void) {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, M452Write);
}

void Mapper452_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, TRUE, FALSE);
	info->Reset = Latch_RegReset;
	info->Power = M452Power;
}
