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
 */

#include "mapinc.h"

static uint8 regs[2];
static uint8 hrd_flag;

static SFORMAT StateRegs[] =
{
	{ &hrd_flag, 1, "DPSW" },
	{ regs, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8 prg = (regs[1] >> 5) & 7;
	uint8 outer_chr = ((regs[0] >> 3) & 8) | (regs[1] & 7);
	if (regs[1] & 0x10) {
		setprg32(0x8000, prg >> 1);
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	}
	if (regs[0] & 0x80) {
		setchr8(outer_chr);
	} else {
		setchr8((outer_chr & ~3) | (regs[0] & 3));
	}
	setmirror(((regs[1] >> 3) & 1) ^ 1);
}

static DECLFR(M057Read) {
	return hrd_flag;
}

static DECLFW(M057Write) {
	if (A & 0x2000) {
		regs[(A >> 11) & 1] = (regs[(A >> 11) & 1] & ~0x40) | (V & 0x40);
	} else {
		regs[(A >> 11) & 1] = V;
	}
	Sync();
}

static void M057Power(void) {
	regs[1] = regs[0] = 0; /* Always reset to menu */
	hrd_flag = 1; /* YH-xxx "Olympic" multicarts disable the menu after one selection */
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, M057Write);
	SetReadHandler(0x6000, 0x6000, M057Read);
	Sync();
}

static void M057Reset(void) {
	regs[1] = regs[0] = 0; /* Always reset to menu */
	hrd_flag++;
	hrd_flag &= 3;
	FCEU_printf("Select Register = %02x\n", hrd_flag);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper057_Init(CartInfo *info) {
	info->Power = M057Power;
	info->Reset = M057Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
