/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * GG1 boards, similar to T-262, with no Data latch
 *
 */

#include "mapinc.h"
#include "latch.h"

static uint8 reset;
static SFORMAT StateRegs[] =
{
	{ &reset, 1, "PADS" },
	{ 0 }
};

static void Sync(void) {
	uint32 bank = (latch.addr >> 2) & 0x1F;
	switch (((latch.addr >> 8) & 2) | ((latch.addr >> 7) & 1)) {
	case 0:
		setprg16(0x8000, bank);
		setprg16(0xC000, 0);	
		break;
	case 1:
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
		break;
	case 2:
	case 3:
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | 7);	
		break;
	}

	setchr8(0);
	setmirror(((latch.addr & 2) >> 1) ^ 1);
}

static DECLFR(M301Read) {
	if ((latch.addr & 0x100) && (PRGsize[0] <= (512 * 1024))) {
		A = (A & 0xFFFE) + reset;
	}
	return CartBR(A);
}

static void M301Power(void) {	
	reset = 0;
	LatchPower();
}

static void M301Reset(void) {
	reset = !reset;
	LatchHardReset();
}

void Mapper301_Init(CartInfo *info) {
	Latch_Init(info, Sync, M301Read, 0, 0);
	info->Power = M301Power;
	info->Reset = M301Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
