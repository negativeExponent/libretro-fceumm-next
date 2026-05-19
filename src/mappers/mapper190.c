/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2017 FCEUX Team
 *  Copyright (C) 2023-2025-2026 negativeExponent
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
 * Magic Kid GooGoo
 */

#include "mapinc.h"

static struct {
	uint8_t prg, chr[4];
} m190;

static SFORMAT StateRegs[] =  {
	{ &m190.prg, 1, "PREG" },
	{ m190.chr, 4, "CREG" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg16(0x8000, m190.prg);
}

static void SyncCHR(void) {
	setchr2(0x0000, m190.chr[0]);
	setchr2(0x0800, m190.chr[1]);
	setchr2(0x1000, m190.chr[2]);
	setchr2(0x1800, m190.chr[3]);
}

static DECLFW(WritePRG) {
	m190.prg = ((A >> 11) & 0x08) | (V & 0x07);
	setprg16(0x8000, ((A >> 11) & 0x08) | (V & 0x07));
}

static DECLFW(WriteCHR) {
	m190.chr[A & 0x03] = V;
	setchr2(0x800 * (A & 0x03), V);
}

static void Power(void) {
	memset(&m190, 0, sizeof(m190));

	SyncPRG();
	SyncCHR();

	setprg8r(0x10, 0x6000, 0);
	setprg16(0xC000, 0);
	setmirror(MI_V);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(0x2000 >> 10, 0x6000, WRAM);

	SetWriteHandler(0x8000, 0x9FFF, WritePRG);
	SetWriteHandler(0xA000, 0xBFFF, WriteCHR);
	SetWriteHandler(0xC000, 0xEFFF, WritePRG);
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
}

void Mapper190_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAM = (uint8_t *)FCEU_gmalloc(0x2000);
	SetupCartPRGMapping(0x10, WRAM, 0x2000, 1);
	AddExState(WRAM, 0x2000, 0, "WRAM");
}
