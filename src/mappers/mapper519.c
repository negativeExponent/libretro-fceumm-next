/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 CaH4e3
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
 */

/* NES 2.0 Mapper 519 
 * UNIF board name UNL-EH8813A
 */

#include "mapinc.h"
#include "latch.h"

static uint8 lock, hw_mode, chr, ram[4];

static SFORMAT StateRegs[] = {
	{ &hw_mode, 1, "HWMO" },
	{ &lock, 1, "LOCK" },
	{ &chr, 1, "CREG" },
	{ 0 }
};

static void Sync(void) {
	if (lock == 0) {
		uint8 prg = (latch.addr & 0x3F);
		
		if (latch.addr & 0x80) {
			setprg16(0x8000, prg);
			setprg16(0xC000, prg);
		} else {
			setprg32(0x8000, prg >> 1);
		}

		setmirror(((latch.data >> 7) & 1) ^ 1);

		lock = (latch.addr & 0x100) >> 8;
		chr = latch.data & 0x7C;
	}

	setchr8(chr | (latch.data & 0x03));
}

static DECLFR(M519ReadRAM) {
	return ram[A & 0x03];
}

static DECLFW(M519WriteRAM) {
	ram[A & 0x03] = V & 0x0F;
}

static DECLFR(M519Read) {
	if (latch.addr & 0x40) {
		A = (A & 0xFFF0) + hw_mode;
	}
	return CartBR(A);
}

static void M519Power(void) {
	hw_mode = lock = 0;
	Latch_Power();
	SetReadHandler(0x5800, 0x5FFF, M519ReadRAM);
	SetWriteHandler(0x5800, 0x5FFF, M519WriteRAM);
}

static void M519Reset(void) {
	lock = 0;
	hw_mode = (hw_mode + 1) & 0xF;
	FCEU_printf("Hardware Switch is %01X\n", hw_mode);
	Latch_RegReset();
}

void Mapper519_Init(CartInfo *info) {
	Latch_Init(info, Sync, M519Read, 0, 0);
	info->Reset = M519Reset;
	info->Power = M519Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
