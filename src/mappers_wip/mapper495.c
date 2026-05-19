/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"

static struct {
	uint8_t prg[3];
	uint8_t chr[4];
	uint8_t latch[2];
} m495;

static SFORMAT StateRegs[] = {
	{ m495.prg,   3, "PREG" },
	{ m495.chr,   4, "CREG" },
	{ m495.latch, 2, "LATC" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m495.prg[0]);
	setprg8(0xA000, m495.prg[1]);
	setprg8(0xC000, m495.prg[2]);
	setprg8(0xE000, 0xFF);
}

static void SyncCHR(void) {
	setchr4(0x0000, m495.chr[0x00 | m495.latch[0]]);
	setchr4(0x1000, m495.chr[0x02 | m495.latch[1]]);
}

static void SyncMirror(void) {
	uint8_t mirr = m495.chr[m495.latch[0]] >> 6;
	switch (mirr) {
	case 0: setmirrorw(0, 0, 0, 1); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_V); break;
	case 3: setmirror(MI_1); break;
	}
}

static void PPUHook(uint32_t A) {
	if ((A & 0x2FF0) == 0xFD0 || (A & 0x2FF0) == 0xFE0) {
		m495.latch[(A >> 12) & 0x01] = ((A & 0x20) ? 0 : 1);
		SyncCHR();
		SyncMirror();
	}
}

static DECLFW(WriteReg) {
	if (A < 0xE000) {
		m495.prg[(A >> 13) & 0x03] = V;
		SyncPRG();
	} else {
		m495.chr[(A >> 10) & 0x03] = V;
		SyncCHR();
		SyncMirror();
	}
}

static void Reset(void) {
	memset(&m495, 0, sizeof(m495));
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m495, 0, sizeof(m495));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper495_Init(CartInfo *info) {
	AddExState(StateRegs, ~0, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	PPU_hook = PPUHook;
	GameStateRestore = StateRestore;
	
}
