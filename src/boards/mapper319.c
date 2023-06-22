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
 */

#include "mapinc.h"
#include "latch.h"

static uint8 reg[2];
static uint8 pad;

static SFORMAT StateRegs[] =
{
	{ reg, 2, "REG" },
	{ &pad, 1, "PAD" },
	{ 0 }
};

static void M319Sync(void) {
	uint8 bank;
	uint8 mask;

	if (iNESCart.CRC32 == 0xE5B9AB1F || iNESCart.PRGCRC32 == 0xC25FD362) {
		/* The publicly-available UNIF (UNL-HP898F) ROM file of Prima Soft 9999999-in-1 has
		 * the order of the 16 KiB PRG-ROM banks slightly mixed up, so that the
		 * PRG A14 mode bit operates on A16 instead of A14. To obtain the
		 * correct bank order, use UNIF 16 KiB PRG banks 0, 4, 1, 5, 2, 6, 3, 7.
		 */
		bank = (reg[1] >> 3) & 7;
		mask = (reg[1] >> 4) & 4;
		setprg16(0x8000, bank & ~mask);
		setprg16(0xC000, bank |  mask);
	} else {
		bank = ((reg[1] >> 2) & 0x06) | ((reg[1] >> 5) & 0x01);
		mask = (reg[1] >> 6) & 0x01;
		setprg16(0x8000, (bank & ~mask));
		setprg16(0xC000, (bank |  mask));
	}

	bank = reg[0] >> 4;
	mask = (reg[0] << 2) & 0x04;
	setchr8((bank & ~mask) | ((latch.data << 2) & mask));
	setmirror(reg[1] >> 7);
}

static DECLFR(M319ReadPad) {
	return pad;
}

static DECLFW(M319WriteReg) {
	switch (A & 0x04) {
	case 0x00:
		reg[0] = V;
		M319Sync();
		break;
	case 0x04:
		reg[1] = V;
		M319Sync();
		break;
	}
}

static void M319Reset(void) {
	reg[0] = reg[1] = 0;
	pad ^= 0x40;
	M319Sync();
}

static void M319Power(void) {
	reg[0] = reg[1] = pad = 0;
	LatchPower();
	SetReadHandler(0x5000, 0x5FFF, M319ReadPad);
	SetWriteHandler(0x6000, 0x7FFF, M319WriteReg);
}

void Mapper319_Init(CartInfo *info) {
	Latch_Init(info, M319Sync, NULL, 0, 0);
	info->Power = M319Power;
	info->Reset = M319Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
