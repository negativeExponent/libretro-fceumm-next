/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* FDS Conversion
 * NES 2.0 Mapper 309 is used for Whirlwind Manu's ROM cartridge conversion
 * of game 愛戦士ニコル (Ai Senshi Nicol, cartridge code LH51).
 * Its UNIF board name is UNL-LH51.
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_309
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t reg[2];
} m309;

static SFORMAT StateRegs[] = {
	{ m309.reg, 2, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);

	setprg8(0x8000, m309.reg[0]);
	setprg8(0xA000, 0xFD);
	setprg8(0xC000, 0xFE);
	setprg8(0xE000, 0xFF);
}

static void SyncCHR(void) {
	setchr8(0);
	setmirror(((m309.reg[1] >> 3) & 0x01) ^ 0x01);
}

static DECLFW(WritePRG) {
	m309.reg[0] = V;
	SyncPRG();
}

static DECLFW(WriteMirror) {
	m309.reg[1] = V;
	SyncCHR();
}

static void Power(void) {
	FDSSound_Power();
	SyncPRG();
	SyncCHR();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0xF000, 0xFFFF, WriteMirror);

	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
}

void Mapper309_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
