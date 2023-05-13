/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 274 is used for the 90-in-1 Hwang Shinwei multicart.
 * Its UNIF board name is BMC-80013-B.
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_274 */

 /* 2020-03-22 - Update support for Cave Story II, FIXME: Arabian does not work for some reasons... */

#include "mapinc.h"

static uint8 regs[2], extra_chip;

static SFORMAT StateRegs[] =
{
	{ regs, 2, "REGS" },
	{ &extra_chip, 1, "XTRA" },
	{ 0 }
};

static void Sync(void) {
	if (extra_chip) {
		setprg16(0x8000, 0x80 | (regs[0] & ((ROM_size - 1) & 0x0F)));
	} else {
		setprg16(0x8000, (regs[1] & 0x70) | (regs[0] & 0x0F));
	}
	setprg16(0xC000, regs[1] & 0x7F);
	setchr8(0);
	setmirror(((regs[0] >> 4) & 1) ^ 1);
}

static DECLFW(BMC80013BWrite8) {
	regs[0] = V;
	Sync();
}

static DECLFW(BMC80013BWriteA) {
	regs[1] = V;
	extra_chip = (A & 0x4000) == 0;
	Sync();
}

static void BMC80013BPower(void) {
	regs[0] = regs[1] = 0;
	extra_chip = 1;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, BMC80013BWrite8);
	SetWriteHandler(0xA000, 0xFFFF, BMC80013BWriteA);
}

static void BMC80013BReset(void) {
	regs[0] = regs[1] = 0;
	extra_chip = 1;
	Sync();
}

static void BMC80013BRestore(int version) {
	Sync();
}

void BMC80013B_Init(CartInfo *info) {
	info->Power = BMC80013BPower;
	info->Reset = BMC80013BReset;
	GameStateRestore = BMC80013BRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
