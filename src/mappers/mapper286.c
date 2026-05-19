/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[4];
	uint8_t mirror;
} m286;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m286.prg, 4, "PREG" },
	{ m286.chr, 4, "CREG" },
	{ &m286.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m286.prg[0]);
	setprg8(0xA000, m286.prg[1]);
	setprg8(0xC000, m286.prg[2]);
	setprg8(0xE000, m286.prg[3]);
}

static void SyncCHR(void) {
	setchr2(0x0000, m286.chr[0]);
	setchr2(0x0800, m286.chr[1]);
	setchr2(0x1000, m286.chr[2]);
	setchr2(0x1800, m286.chr[3]);
}

static void SyncMirror(void) {
	setmirror((m286.mirror & 0x01) ^ 0x01);
}

static DECLFW(WriteCHR) {
	m286.chr[(A & 0xC00) >> 10] = A & 0x1F;
	SyncCHR();
}

static DECLFW(WritePRG) {
	if (A & (1 << (dipsw + 4))) {
		m286.prg[(A & 0xC00) >> 10] = A & 0x0F;
		SyncPRG();
	}
}

static DECLFW(WriteMirror) {
	m286.mirror = V;
	SyncMirror();
}

static void Reset(void) {
	memset(&m286, 0, sizeof(m286));
	m286.prg[0] = 0x0C;
	m286.prg[1] = 0x0D;
	m286.prg[2] = 0x0E;
	m286.prg[3] = 0x0F;
	dipsw++;
	dipsw &= 3;
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m286, 0, sizeof(m286));
	m286.prg[0] = 0x0C;
	m286.prg[1] = 0x0D;
	m286.prg[2] = 0x0E;
	m286.prg[3] = 0x0F;
	dipsw = 0;
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, WriteCHR);
	SetWriteHandler(0xA000, 0xBFFF, WritePRG);
	SetWriteHandler(0xC000, 0xCFFF, WriteMirror);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper286_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
