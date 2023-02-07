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
	if (reg[1] & 0x40)
		setprg32(0x8000, (reg[1] >> 3) & 3);
	else {
		setprg16(0x8000, ((reg[1] >> 2) & 6) | ((reg[1] >> 5) & 1));
		setprg16(0xC000, ((reg[1] >> 2) & 6) | ((reg[1] >> 5) & 1));
	}
	setchr8(((reg[0] >> 4) & ~((reg[0] << 2) & 4)) | ((latch.data << 2) & ((reg[0] << 2) & 4)));
	setmirror(reg[1] >> 7);
}

static DECLFR(M319ReadPad) {
	return pad;
}

static DECLFW(M319WriteReg) {
	reg[A >> 2 & 1] = V;
	M319Sync();
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
	Latch_Init(info, M319Sync, M319ReadPad, 0, 0);
	info->Power = M319Power;
	info->Reset = M319Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
