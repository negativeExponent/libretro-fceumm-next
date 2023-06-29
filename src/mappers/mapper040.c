/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * FDS Conversion
 *
 */

#include "mapinc.h"

static uint8 reg;
static uint32 IRQCount, IRQa;
static uint8 outer_bank;
static uint8 submapper;

static SFORMAT StateRegs[] =
{
	{ &IRQCount, 4, "IRQC" },
	{ &IRQa, 4, "IRQA" },
	{ &reg, 1, "REG" },
	{ &outer_bank, 1, "OUTB" },
	{ 0 }
};

static void Sync(void) {
	if (outer_bank & 8) {
		if (outer_bank & 0x10) {
			setprg32(0x8000, 2 | (outer_bank >> 6));
		} else {
			setprg16(0x8000, 4 | (outer_bank >> 5));
			setprg16(0xC000, 4 | (outer_bank >> 5));
		}
	} else {
		setprg8(0x6000, 6);
		setprg8(0x8000, 4);
		setprg8(0xa000, 5);
		setprg8(0xc000, reg & 7);
		setprg8(0xe000, 7);
	}
	setchr8((outer_bank >> 1) & 3);
	setmirror((outer_bank & 1) ^ 1);
}

static DECLFW(M040Write) {
	switch (A & 0xe000) {
		case 0x8000:
			IRQa = 0;
			IRQCount = 0;
			X6502_IRQEnd(FCEU_IQEXT);
			break;
		case 0xa000:
			IRQa = 1;
			break;
		case 0xc000:
			if (submapper == 1) {
				outer_bank = A & 0xFF;
				Sync();
				
			}
			break;
		case 0xe000:
			reg = V;
			Sync();
			break;
	}
}

static void M040Power(void) {
	reg = 0;
	outer_bank = 0;
	IRQCount = IRQa = 0;
	Sync();
	SetReadHandler(0x6000, 0xffff, CartBR);
	SetWriteHandler(0x8000, 0xffff, M040Write);
}

static void M040Reset(void) {
	reg = 0;
	outer_bank = 0;
	IRQCount = IRQa = 0;
	Sync();
 }

static void FP_FASTAPASS(1) M040IRQHook(int a) {
	if (IRQa) {
		if (IRQCount < 4096)
			IRQCount += a;
		else {
			IRQa = 0;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper040_Init(CartInfo *info) {
	info->Reset = M040Reset;
	info->Power = M040Power;
	MapIRQHook = M040IRQHook;
	GameStateRestore = StateRestore;
	submapper = info->submapper;
	AddExState(&StateRegs, ~0, 0, 0);
}
