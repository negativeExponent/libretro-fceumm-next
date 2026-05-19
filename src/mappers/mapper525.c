/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 */

/* NES Mapper 525 - UNL-KS7021A
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_525
 * NES 2.0 Mapper 525 is used for a bootleg version of versions of Contra and 月風魔伝 (Getsu Fūma Den).
 * Its similar to Mapper 23 Submapper 3) with non-nibblized CHR-ROM bank registers.
 */

#include "mapinc.h"

static struct {
	uint8_t prg;
	uint8_t chr[8];
	uint8_t mirror;
} m525;

static SFORMAT StateRegs[] = {
	{ m525.chr, 8, "CHRR" },
	{ &m525.prg, 1, "PRGR" },
	{ &m525.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg16(0x8000, m525.prg >> 1);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr1(0x0000, m525.chr[0]);
	setchr1(0x0400, m525.chr[1]);
	setchr1(0x0800, m525.chr[2]);
	setchr1(0x0C00, m525.chr[3]);
	setchr1(0x1000, m525.chr[4]);
	setchr1(0x1400, m525.chr[5]);
	setchr1(0x1800, m525.chr[6]);
	setchr1(0x1C00, m525.chr[7]);
}

static void SyncMirror(void) {
	setmirror((m525.mirror & 0x01) ^ 0x01);
}

static DECLFW(WritePRG) {
	m525.prg = V;
	SyncPRG();
}

static DECLFW(WriteMirror) {
	m525.mirror = V;
	SyncMirror();
}

static DECLFW(WriteCHR) {
	m525.chr[A & 0x07] = V;
	SyncCHR();
}

static void Power(void) {
	memset(&m525, 0, sizeof(m525));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, WriteMirror);
	SetWriteHandler(0xB000, 0xBFFF, WriteCHR);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper525_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
