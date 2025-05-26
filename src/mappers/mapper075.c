/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2023-2025 negativeExponent
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
 * Konami VRC-1
 *
 */

#include "mapinc.h"

static struct {
	uint8 prg[3], chr[2], mode;
} m075;

static SFORMAT StateRegs[] = {
	{ &m075.mode, 1, "MODE" },
	{ m075.chr, 2, "CREG" },
	{ m075.prg, 3, "PREG" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m075.prg[0]);
	setprg8(0xA000, m075.prg[1]);
	setprg8(0xC000, m075.prg[2]);
	setprg8(0xE000, ~0);
}

static void SyncCHR(void) {
	setchr4(0x0000, (m075.chr[0] & 0x0F) | ((m075.mode & 0x02) << 3));
	setchr4(0x1000, (m075.chr[1] & 0x0F) | ((m075.mode & 0x04) << 2));
}

static void SyncMirror(void) {
	if (iNESCart.mirror == MI_4) {
		setmirror(MI_4);
	} else {
		setmirror((m075.mode & 1) ^ 1);
	}
}

static DECLFW(WritePRG) {
	m075.prg[(A >> 13) & 0x03] = V;
	SyncPRG();
}

static DECLFW(WriteMode) {
	m075.mode = V;
	SyncCHR();
	SyncMirror();
}

static DECLFW(WriteCHR) {
	m075.chr[(A >> 12) & 0x01] = V;
	SyncCHR();
}

static void Power(void) {
	memset(&m075, 0, sizeof(m075));

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, WriteMode);
	SetWriteHandler(0xA000, 0xDFFF, WritePRG);
	SetWriteHandler(0xE000, 0xFFFF, WriteCHR);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper075_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
