/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2020
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

/* added 2020-2-15
 * Street Fighter 3, Mortal Kombat II, Dragon Ball Z 2, Mario & Sonic 2 (JY-016)
 * 1995 Super HIK 4-in-1 (JY-016), 1995 Super HiK 4-in-1 (JY-017)
 * submapper 1 - Super Fighter III
 * NOTE: nesdev's notes for IRQ is different that whats implemented here
 */

#include "mapinc.h"

static uint8 cregs[4], pregs[2];
static uint8 outerbank;
static uint8 mirr;

static uint8 IRQa;
static uint8 IRQp;
static uint8 IRQCount;
static int16 IRQCount2;

static SFORMAT StateRegs[] =
{
	{ cregs, 4, "CREG" },
	{ pregs, 2, "PREG" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQp, 1, "IRQP" },
	{ &IRQCount, 1, "IRQC" },
	{ &IRQCount2, 4, "IRQ2" },
	{ &outerbank, 1, "OUTB" },
	{ &mirr, 1, "MIRR" },
	{ 0 }
};

static void Sync(void) {
	/*	FCEU_printf("P0:%02x P1:%02x outerbank:%02x\n", pregs[0], pregs[1], outerbank);*/
	setprg8(0x8000, pregs[0] | (outerbank & 6) << 3);
	setprg8(0xa000, pregs[1] | (outerbank & 6) << 3);
	setprg8(0xc000,      0xE | (outerbank & 6) << 3);
	setprg8(0xe000,      0xF | (outerbank & 6) << 3);

	setchr2(0x0000, cregs[0] | (outerbank & 1) << 8);
	setchr2(0x0800, cregs[1] | (outerbank & 1) << 8);
	setchr2(0x1000, cregs[2] | (outerbank & 1) << 8);
	setchr2(0x1800, cregs[3] | (outerbank & 1) << 8);

	if (iNESCart.submapper == 1) {
		setmirror(mirr ^ 1);
	}
}

static DECLFW(M091CHRWrite) {
	if (iNESCart.submapper == 1) {
		switch (A & 7) {
		case 0:
		case 1:
		case 2:
		case 3:
			cregs[A & 3] = V;
			Sync();
			break;
		case 4:
		case 5:
			mirr = V & 1;
			Sync();
			break;
		case 6:
			IRQCount2 = (IRQCount2 & 0xFF00) | V;
			break;
		case 7:
			IRQCount2 = (IRQCount2 & 0x00FF) | (V << 8);
			break;
		}
	} else {
		cregs[A & 3] = V;
		Sync();
	}
}

static DECLFW(M091IRQWrite) {
	switch (A & 3) {
	case 0:
	case 1:
		pregs[A & 1] = V;
		Sync();
		break;
	case 2:
		IRQa = IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 3:
		IRQa = 1; IRQp = 3;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static DECLFW(M091OuterBankWrite) {
	outerbank = A & 7;
	Sync();
}

static void M091Power(void) {
	Sync();
	SetReadHandler(0x8000, 0xffff, CartBR);
	SetWriteHandler(0x6000, 0x6fff, M091CHRWrite);
	SetWriteHandler(0x7000, 0x7fff, M091IRQWrite);
	SetWriteHandler(0x8000, 0x9fff, M091OuterBankWrite);
}

static void M091HBHook(void) {
	if (IRQCount < 8 && IRQa) {
		IRQCount++;
		if (IRQCount >= 8) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void M091IRQHook(int a) {
	IRQp += a;
	if (IRQp >= 4) {
		IRQp -= 4;
		IRQCount2 -= 5;
		if (IRQCount2 <= 0) {
			if (IRQa) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper091_Init(CartInfo *info) {
	info->Power = M091Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	if (info->submapper == 1) {
		MapIRQHook = M091IRQHook;
	} else {
		GameHBIRQHook = M091HBHook;
	}
}
