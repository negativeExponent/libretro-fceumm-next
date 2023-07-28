/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

/* Mapper 053 - BMC-Supervision16in1 */

#include "mapinc.h"

static uint8 cmd0, cmd1;
static SFORMAT StateRegs[] =
{
	{ &cmd0, 1, "L1" },
	{ &cmd1, 1, "L2" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg8(0x6000, (((cmd0 & 0xF) << 4) | 0xF) + 4);
	if (cmd0 & 0x10) {
			setprg16(0x8000, (((cmd0 & 0xF) << 3) | (cmd1 & 7)) + 2);
			setprg16(0xc000, (((cmd0 & 0xF) << 3) | 7) + 2);
	} else
		setprg32(0x8000, 0);
	setmirror(((cmd0 & 0x20) >> 5) ^ 1);
}

static DECLFW(M053Write6) {
	if (!(cmd0 & 0x10)) {
		cmd0 = V;
		Sync();
	}
}

static DECLFW(M053Write8) {
	cmd1 = V;
	Sync();
}

static void M053Power(void) {
	SetWriteHandler(0x6000, 0x7FFF, M053Write6);
	SetWriteHandler(0x8000, 0xFFFF, M053Write8);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	cmd0 = cmd1 = 0;
	Sync();
}

static void M053Reset(void) {
	cmd0 = cmd1 = 0;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper053_Init(CartInfo *info) {
	info->Power = M053Power;
	info->Reset = M053Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
