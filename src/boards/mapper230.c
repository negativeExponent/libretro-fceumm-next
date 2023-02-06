/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2009 qeed
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
 * 22 + Contra Reset based custom mapper...
 *
 */

#include "mapinc.h"
#include "latch.h"

static uint8 reset;
static SFORMAT StateRegs[] =
{
	{ &reset, 1, "RST" },
	{ 0 }
};

static void Sync(void) {
	if (reset) {  /* Contra mode */
		setprg16(0x8000, latch.data & 7);
		setprg16(0xC000, 7);
		setmirror(MI_V);
	} else { /* multicart mode */
		uint32 bank = (latch.data & 0x1F) + 8;
		if (latch.data & 0x20) {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		} else
			setprg32(0x8000, bank >> 1);
		setmirror((latch.data >> 6) & 1);
	}
	setchr8(0);
}

static void M230Reset(void) {
	reset ^= 1;
	LatchHardReset();
}

static void M230Power(void) {
	reset = 0;
	LatchPower();
}

void Mapper230_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = M230Power;
	info->Reset = M230Reset;
	AddExState(StateRegs, ~0, 0, 0);
}
