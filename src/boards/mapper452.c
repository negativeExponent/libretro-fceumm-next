/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

/* DS-9-27. Absolutely insane PCB that overlays 8 KiB of WRAM into a selectable position between $8000 and $E000. */

#include "mapinc.h"

static uint8 *WRAM;
static uint32 WRAMSIZE;
static uint8 latch_data;
static uint16 latch_addr;

static void Sync(void) {
	uint8 wramBank = ((latch_data >> 3) & 6) | 8;
	uint16 prgbank = latch_addr >> 1;
	if (latch_data & 2) {
		setprg8(0x8000, prgbank);
		setprg8(0xA000, prgbank);
		setprg8(0xC000, prgbank);
		setprg8(0xE000, prgbank);
		setprg8r(0x10, (wramBank ^ 4) << 12, 0);
	} else if (latch_data & 8) {
		setprg8(0x8000, (prgbank & ~1) | 0);
		setprg8(0xA000, (prgbank & ~1) | 1);
		setprg8(0xC000, (prgbank & ~1) | 2);
		setprg8(0xE000, 3 | (prgbank & ~1)
			| (latch_data & 4)
			| ((latch_data & 0x04) && (latch_data & 0x40) ? 8 : 0));
	} else {
		setprg16(0x8000, latch_addr >> 2);
		setprg16(0xC000, 0);
	}
	setprg8r(0x10, (wramBank << 12), 0);
	setchr8(0);
	setmirror((latch_data & 1) ^ 1);
}

static DECLFW(M452Write) {
	latch_addr = A & 0xFFF;
	latch_data = V;
	Sync();
	/* Do not relay to CartBW, as RAM mapped to locations other than $8000-$DFFF are not write-enabled. */
}

static void M452Reset(void) {
	latch_addr = latch_data = 0;
	Sync();
}

static void M452Power(void) {
	latch_addr = latch_data = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xDFFF, M452Write);
	SetWriteHandler(0xE000, 0xFFFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void M452Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper452_Init(CartInfo *info) {
	info->Reset      = M452Reset;
	info->Power      = M452Power;
	info->Close      = M452Close;
	GameStateRestore = StateRestore;

	WRAMSIZE         = 8192;
	WRAM             = (uint8 *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(&latch_addr, 2, 0, "ADDR");
	AddExState(&latch_data, 1, 0, "DATA");
}
