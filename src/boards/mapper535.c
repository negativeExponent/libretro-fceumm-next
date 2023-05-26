/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 * NES 2.0 Mapper 535 - UNL-M535
 * FDS Conversion - Nazo no Murasamejō
 *
 */

#include "mapinc.h"
#include "../fds_apu.h"

static uint8 reg, IRQa;
static int32 IRQCount;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ &reg, 1, "REG" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQCount, 4, "IRQC" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	setprg8(0x6000, reg);
	setprg8(0x8000, 0xc);
	setprg4(0xa000, (0xd << 1));
	setprg2(0xb000, (0xd << 2) + 2);
	setprg2r(0x10, 0xb800, 4);
	setprg2r(0x10, 0xc000, 5);
	setprg2r(0x10, 0xc800, 6);
	setprg2r(0x10, 0xd000, 7);
	setprg2(0xd800, (0xe << 2) + 3);
	setprg8(0xe000, 0xf);
}

static DECLFW(M535RamWrite) {
	WRAM[(A - 0xB800) & 0x1FFF] = V;
}

static DECLFW(M535Write) {
	reg = V;
	Sync();
}

static DECLFW(M535IRQaWrite) {
	IRQa = V & 2;
	IRQCount = 0;
	if (!IRQa)
		X6502_IRQEnd(FCEU_IQEXT);
}

static void FP_FASTAPASS(1) M535IRQHook(int a) {
	if (IRQa) {
		IRQCount += a;
		if (IRQCount > 7560)
			X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void M535Power(void) {
	FDSSoundPower();
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xB800, 0xD7FF, M535RamWrite);
	SetWriteHandler(0xE000, 0xEFFF, M535IRQaWrite);
	SetWriteHandler(0xF000, 0xFFFF, M535Write);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M535Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper535_Init(CartInfo *info) {
	info->Power = M535Power;
	info->Close = M535Close;
	MapIRQHook = M535IRQHook;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(&StateRegs, ~0, 0, 0);
}
