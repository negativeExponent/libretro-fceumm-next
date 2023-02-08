/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2022
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	uint32 S = latch.addr & 1;
	uint32 p = ((latch.addr >> 2) & 0x1F) + ((latch.addr & 0x100) >> 3) + ((latch.addr & 0x400) >> 4);
	uint32 L = (latch.addr >> 9) & 1;
	uint32 p_8000 = p;

	if ((latch.addr >> 11) & 1)
		p_8000 = (p & 0x7E) | (latch.data & 7);

	if ((latch.addr >> 7) & 1) {
		if (S) {
			setprg32(0x8000, p >> 1);
		} else {
			setprg16(0x8000, p_8000);
			setprg16(0xC000, p);
		}
	} else {
		if (S) {
			if (L) {
				setprg16(0x8000, p_8000 & 0x7E);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p_8000 & 0x7E);
				setprg16(0xC000, p & 0x78);
			}
		} else {
			if (L) {
				setprg16(0x8000, p_8000);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p_8000);
				setprg16(0xC000, p & 0x78);
			}
		}
	}

	if ((latch.addr & 0x80) == 0x80)
		/* CHR-RAM write protect hack, needed for some multicarts */
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);

	setmirror(((latch.addr >> 1) & 1) ^ 1);
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
}

static DECLFW(M375Write) {
	if (latch.addr & 0x800) {
		latch.data = V;
	} else {
		latch.addr = A;
		latch.data = V;
	}
	Sync();
}

static void M375Power(void) {
	LatchPower();
	SetWriteHandler(0x8000, 0xFFFF, M375Write);
}

void Mapper375_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 1, 0);
	info->Power = M375Power;
	info->Reset = LatchHardReset;
}
