/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* FDS Conversion - Kid Icarus (パルテナの鏡) (Parthena) */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t prg;
	uint8_t mirror;
} m539;

static SFORMAT StateRegs[] = {
	{ &m539.prg, 1, "PREG" },
	{ &m539.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0xA000, m539.prg & 0x0F);
}

static void SyncMirror(void){
	setmirror(((m539.mirror & 8) >> 3) ^ 1);
}

static DECLFR(ReadWRAM) {
	A = (((A) & 0x1FFF) | (((A) < 0xC000) ? 0x1000 : 0x0000) | (((A) < 0x8000) ? 0x800 : 0x000));
	return WRAM[A];
}

static DECLFW(WriteWRAM) {
	A = (((A) & 0x1FFF) | (((A) < 0xC000) ? 0x1000 : 0x0000) | (((A) < 0x8000) ? 0x800 : 0x000));
	WRAM[A] = V;
}

static DECLFW(WritePRG) {
	m539.prg = V;
	SyncPRG();
}

static DECLFW(WriteMirror) {
	if ((A & 0x25) == 0x25) {
		m539.mirror = V;
		SyncMirror();
	}
}

static void Power(void) {
	memset(&m539, 0, sizeof(m539));
	FDSSound_Power();

	setprg8(0x6000, 0x0D);
	setprg8(0x8000, 0x0C);
	setprg8(0xA000, m539.prg & 0x0F);
	setprg8(0xC000, 0x0E);
	setprg8(0xE000, 0x0F);
	setchr8(0);

	SetReadHandler(0x6000, 0xFFFF, CartBR);

	SetWriteHandler(0xA000, 0xAFFF, WritePRG);
	SetWriteHandler(0xF000, 0xFFFF, WriteMirror);

	/* Certain ranges in the CPU address space are overlaid with portions of 8 KiB of PRG-RAM as follows:
	 * CPU $6000-$60FF
	 * CPU $6200-$62FF
	 * CPU $6400-$65FF
	 * CPU $8200-$82FF
	 * CPU $C000-$D1FF
	 * CPU $DF00-$DFFF
	 */

	SetReadHandler(0x6000, 0x60FF, ReadWRAM);
	SetReadHandler(0x6200, 0x62FF, ReadWRAM);
	SetReadHandler(0x6400, 0x65FF, ReadWRAM);
	SetReadHandler(0x8200, 0x82FF, ReadWRAM);
	SetReadHandler(0xC000, 0xD1FF, ReadWRAM);
	SetReadHandler(0xDF00, 0xDFFF, ReadWRAM);

	SetWriteHandler(0x6000, 0x60FF, WriteWRAM);
	SetWriteHandler(0x6200, 0x62FF, WriteWRAM);
	SetWriteHandler(0x6400, 0x65FF, WriteWRAM);
	SetWriteHandler(0x8200, 0x82FF, WriteWRAM);
	SetWriteHandler(0xC000, 0xD1FF, WriteWRAM);
	SetWriteHandler(0xDF00, 0xDFFF, WriteWRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncMirror();
}

void Mapper539_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
