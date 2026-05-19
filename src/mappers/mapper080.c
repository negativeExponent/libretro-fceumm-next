/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
 *  Copyright (C) 2002 Xodnizel
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

/* iNES Mapper 080 represents most boards using Taito's X1-005 mapper IC, which
 * provides something somewhere between the MMC6 and the Namcot 108 in
 * sophistication.*/

/* iNES Mapper 207 represents the board used for Fudou Myouou Den.
 * It modifies Taito's X1-005 (80) in the exact same way that TLSROM (118) differs
 * from MMC3 (4), by which we mean the X1-005's CHR A17 output is connected to
 * CIRAM A10. Thus, it achieves 1scA/1scB/H controllable mirroring instead of H/V.
 */

#include "mapinc.h"

static struct {
	uint8_t prg[3], chr[6], mirror;
	uint8_t internalRAM[128];
} m080;

static SFORMAT StateRegs[] = {
	{ m080.prg, 3, "PREG" },
	{ m080.chr, 6, "CREG" },
	{ &m080.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m080.prg[0]);
	setprg8(0xA000, m080.prg[1]);
	setprg8(0xC000, m080.prg[2]);
	setprg8(0xE000, ~0);
}

static void SyncCHR(void) {
	setchr2(0x0000, m080.chr[0] >> 1);
	setchr2(0x0800, m080.chr[1] >> 1);
	setchr1(0x1000, m080.chr[2]);
	setchr1(0x1400, m080.chr[3]);
	setchr1(0x1800, m080.chr[4]);
	setchr1(0x1C00, m080.chr[5]);
}

static void SyncMirror(void) {
	if (iNESCart.mapper == 207) {
		setmirrorw(m080.chr[0] >> 7, m080.chr[0] >> 7, m080.chr[1] >> 7, m080.chr[1] >> 7);
	} else {
		setmirror(m080.mirror & 0x01);
	}
}

static DECLFR(ReadInternalRAM) {
	return m080.internalRAM[A & 0x7F];
}

static DECLFW(WriteInternalRAM) {
	m080.internalRAM[A & 0x7F] = V;
}

static DECLFW(WriteReg) {
	switch (A) {
	case 0x7EF0:
	case 0x7EF1:
	case 0x7EF2:
	case 0x7EF3:
	case 0x7EF4:
	case 0x7EF5:
		m080.chr[A & 0x07] = V;
		SyncCHR();
		SyncMirror();
		break;
	case 0x7EF6:
		m080.mirror = V;
		SyncMirror();
		break;
	case 0x7EFA:
	case 0x7EFB:
	case 0x7EFC:
	case 0x7EFD:
	case 0x7EFE:
	case 0x7EFF:
		m080.prg[(A - 0x7EFA) >> 1] = V;
		SyncPRG();
		break;
	}
}

static void Power(void) {
	memset(&m080, 0, sizeof(m080));

	m080.prg[0] = 0x00;
	m080.prg[1] = 0x01;
	m080.prg[2] = 0xFE;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x7EF0, 0x7EFF, WriteReg);

	if (iNESCart.mapper == 80) {
		SetReadHandler(0x7F00, 0x7FFF, ReadInternalRAM);
		SetWriteHandler(0x7F00, 0x7FFF, WriteInternalRAM);
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper080_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	if (info->battery) {
		info->SaveGame[0] = m080.internalRAM;
		info->SaveGameLen[0] = 128;
		AddExState(m080.internalRAM, sizeof(m080.internalRAM), 0, "WRAM");
	}
}
