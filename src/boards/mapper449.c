/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

#include "mapinc.h"

static uint16 latch_addr;
static uint8 latch_data;
static uint8 dipswitch;

static SFORMAT StateRegs[] = {
	{ &latch_addr, 2, "ADDR" },
    { &latch_data, 1, "DATA" },
    { &dipswitch,  1, "DIPS" },
    { 0 }
};

static void Sync(void) {
	uint8 prgbank = latch_addr >> 2 & 0x1F | latch_addr >> 3 & 0x20;
	if (~latch_addr & 0x080) {
		setprg16(0x8000, prgbank);
		setprg16(0xC000, prgbank | 7);
	} else {
		if (latch_addr & 0x001) {
			setprg32(0x8000, prgbank >> 1);
		} else {
			setprg16(0x8000, prgbank);
			setprg16(0xC000, prgbank);
		}
	}
	setchr8(latch_data);
    setmirror((((latch_addr >> 1) & 1) ^ 1));
}

static DECLFR(M449Read) {
	if (latch_addr & 0x200)
		return CartBR(A | dipswitch);
	return CartBR(A);
}

static DECLFW(M449Write) {
	latch_data = V;
	latch_addr = A & 0xFFFF;
	Sync();
}

static void M449Reset(void) {
	dipswitch  = (dipswitch + 1) & 0xF;
	latch_addr = latch_data = 0;
	Sync();
}

static void M449Power(void) {
	dipswitch = latch_addr = latch_data = 0;
	Sync();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetReadHandler(0x8000, 0xFFFF, M449Read);
	SetWriteHandler(0x8000, 0xFFFF, M449Write);
}

void Mapper449_Init(CartInfo *info) {
	info->Power = M449Power;
	info->Reset = M449Reset;
	AddExState(StateRegs, ~0, 0, 0);
}
