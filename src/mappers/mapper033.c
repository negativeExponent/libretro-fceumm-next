/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

/* iNES Mapper 33 - Taito TC0190/TC0350 */
/* iNES Mapper 48 - Taito TC0690/TC190+PAL16R4 */

#include "mapinc.h"

static uint8 regs[8], mirr;
static uint8 IRQa;
static int16 IRQCount, IRQLatch;

static SFORMAT StateRegs[] =
{
	{ regs, 8, "PREG" },
	{ &mirr, 1, "MIRR" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQCount, 2, "IRQC" },
	{ &IRQLatch, 2, "IRQL" },
	{ 0 }
};

static void Sync(void) {
	setprg8(0x8000, regs[0]);
	setprg8(0xA000, regs[1]);
	setprg8(0xC000, ~1);
	setprg8(0xE000, ~0);
	setchr2(0x0000, regs[2]);
	setchr2(0x0800, regs[3]);
	setchr1(0x1000, regs[4]);
	setchr1(0x1400, regs[5]);
	setchr1(0x1800, regs[6]);
	setchr1(0x1C00, regs[7]);
	if (iNESCart.mapper == 33) {
		setmirror(((regs[0] >> 6) & 1) ^ 1);
	} else {
		setmirror(((mirr >> 6) & 1) ^ 1);
	}
}

static DECLFW(M33Write) {
	regs[((A >> 11) & 0x04) | (A & 0x03)] = V;
	Sync();
}

static DECLFW(IRQWrite) {
	switch (A & 0xF003) {
	case 0xC000:
		IRQLatch = V;
		break;
	case 0xC001:
		IRQCount = IRQLatch;
		break;
	case 0xC003:
		IRQa = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xC002:
		IRQa = 1;
		break;
	case 0xE000:
		mirr = V;
		Sync();
		break;
	}
}

static void M33Power(void) {
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, M33Write);
	if (iNESCart.mapper == 48) {
		SetWriteHandler(0xC000, 0xFFFF, IRQWrite);
	}
}

static void IRQHook(void) {
	if (IRQa) {
		IRQCount++;
		if (IRQCount == 0x100) {
			X6502_IRQBegin(FCEU_IQEXT);
			IRQa = 0;
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper033_Init(CartInfo *info) {
	info->Power = M33Power;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper048_Init(CartInfo *info) {
	Mapper033_Init(info);
	GameHBIRQHook = IRQHook;
}

