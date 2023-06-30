/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
 * NES 2.0 Mapper 272 is used for a bootleg implementation of
 * 悪魔城 Special: ぼくDracula君 (Akumajō Special: Boku Dracula-kun).
 *
 * as implemented from
 * https://forums.nesdev.org/viewtopic.php?f=9&t=15302&start=60#p205862
 *
 */

#include "mapinc.h"

static uint8 prg[2];
static uint8 chr[8];
static uint8 mirr;
static uint8 pal_mirr;
static uint8 IRQCount;
static uint8 IRQa;

static uint16 lastAddr;

static SFORMAT StateRegs[] =
{
	{ prg, 2, "PRG" },
	{ chr, 8, "CHR" },
	{ &mirr, 1, "MIRR" },
	{ &IRQCount, 1, "CNTR" },
	{ &pal_mirr, 1, "PALM" },
	{ &IRQa, 1, "CCLK" },
	{ 0 }
};

static void Sync(void) {
	uint8 i;
	setprg8(0x8000, prg[0]);
	setprg8(0xa000, prg[1]);
	setprg16(0xc000, -1);
	for (i = 0; i < 8; ++i) {
		setchr1(0x400 * i, chr[i]);
	}
	switch (pal_mirr) {
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	default:
		switch (mirr) {
		case 0: setmirror(MI_V); break;
		case 1: setmirror(MI_H); break;
		}
	}
}

static DECLFW(M272Write) {
	/* writes to VRC chip */
	switch (A & 0xF000) {
	case 0x8000:
		prg[0] = V;
		break;
	case 0x9000:
		mirr = V & 1;
		break;
	case 0xA000:
		prg[1] = V;
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000: {
		int bank = (((A - 0xB000) >> 11) & 0x06) | ((A >> 1) & 0x01);
		if (A & 0x01) {
			chr[bank] = (chr[bank] & ~0xF0) | (V << 4);
		} else {
			chr[bank] = (chr[bank] & ~0x0F) | (V & 0x0F);
		}
		break;
	}
	}

	/* writes to PAL chip */
	switch (A & 0xC00C) {
	case 0x8004:
		pal_mirr = V & 3;
		break;
	case 0x800c:
		X6502_IRQBegin(FCEU_IQEXT);
		break;
	case 0xc004:
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xc008:
		IRQa = 1;
		break;
	case 0xc00c:
		IRQa = 0;
		IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}

	Sync();
}

static void M272Reset(void) {
	prg[0] = prg[1] = 0;
	chr[0] = chr[1] = chr[2] = chr[3] = 0;
	chr[4] = chr[5] = chr[6] = chr[7] = 0;
	mirr = pal_mirr = 0;
	lastAddr = 0;
	IRQCount = 0;
	IRQa = 0;
	Sync();
}

static void M272Power(void) {
	M272Reset();
	SetWriteHandler(0x8000, 0xFFFF, M272Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void M272Hook(uint32 A) {
	if ((lastAddr & 0x2000) && (~A & 0x2000)) {
		if (IRQa) {
			IRQCount++;
			if (IRQCount == 84) {
				IRQCount = 0;
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
	lastAddr = A;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper272_Init(CartInfo *info) {
	info->Power = M272Power;
	info->Reset = M272Reset;
	PPU_hook = M272Hook;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
