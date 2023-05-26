/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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
 * Mapper 221 - UNL-N625092
 * 700in1 and 400in1 carts
 * 1000-in-1
 */

#include "mapinc.h"

static uint16 cmd, bank;

static SFORMAT StateRegs[] =
{
	{ &cmd, 2, "CMD" },
	{ &bank, 2, "BANK" },
	{ 0 }
};

static void Sync(void) {
	setmirror((cmd & 1) ^ 1);
	setchr8(0);
	if (cmd & 2) {
		if (cmd & 0x100) {
			setprg16(0x8000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | bank);
			setprg16(0xC000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | 7);
		} else {
			setprg16(0x8000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | (bank & 6));
			setprg16(0xC000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | ((bank & 6) | 1));
		}
	} else {
		setprg16(0x8000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | bank);
		setprg16(0xC000, ((cmd & 0x200) >> 3) | ((cmd & 0xfc) >> 2) | bank);
	}
}

static uint16 ass = 0;

static DECLFW(M221WriteCommand) {
	cmd = A;
	if (A == 0x80F8) {
		setprg16(0x8000, ass);
		setprg16(0xC000, ass);
	} else {
		Sync();
	}
}

static DECLFW(M221WriteBank) {
	bank = A & 7;
	Sync();
}

static void M221Power(void) {
	cmd = 0;
	bank = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, M221WriteCommand);
	SetWriteHandler(0xC000, 0xFFFF, M221WriteBank);
}

static void M221Reset(void) {
	cmd = 0;
	bank = 0;
	ass++;
	FCEU_printf("%04x\n", ass);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper221_Init(CartInfo *info) {
	info->Reset = M221Reset;
	info->Power = M221Power;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
