/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
 * Copyright (C) 2023
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

/* NES 2.0 Mapper 320 is used for the Super HiK 6-in-1 A-030 multicart.
 * Basically UxROM with an address-latch-based outer bank register.
 * UNIF board name is BMC-830425C-4391T. Mirroring is hard-wired.
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_320
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	if (latch.addr & 0x10) { /* UNROM */
		setprg16(0x8000, ((latch.addr << 3) & ~7) | (latch.data & 7));
		setprg16(0xC000, ((latch.addr << 3) & ~7) | 7);
	} else { /* UOROM */
		setprg16(0x8000, ((latch.addr << 3) & ~7) | (latch.data & 0x0F));
		setprg16(0xC000, ((latch.addr << 3) & ~7) | 0x0F);
	}
	setchr8(0);
	setmirror(MI_V);
}

static DECLFW(M320Write) {
	if ((A & 0xFFE0) == 0xF0E0) {
		Latch_Write(A, V);
	} else {
		latch.data = V;
		Sync();
	}
}

static void M320Power() {
	Latch_Power();
	SetWriteHandler(0x8000, 0xFFFF, M320Write);
}

void Mapper320_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = M320Power;
	info->Reset = Latch_RegReset;
}
