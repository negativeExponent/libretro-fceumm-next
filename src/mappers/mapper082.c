/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * Taito X1-017 board, battery backed
 * NES 2.0 Mapper 552 represents the actual way the mask ROM is connected and is thus
 * the correct bank order, while iNES Mapper 082 represents the bank order as it was
 * understood before January 2020 when the mapper was reverse-engineered.
 */

#include "mapinc.h"

static struct {
	uint8_t chr[6], prg[3], protect[3], ctrl;
} m082;

static SFORMAT StateRegs[] = {
	{ m082.prg, 3, "PREGS" },
	{ m082.chr, 6, "CREGS" },
	{ m082.protect, 3, "PROT" },
	{ &m082.ctrl, 1, "CTRL" },

	{ 0 }
};

static uint8_t GetPRGBank(uint8_t V) {
	if (iNESCart.mapper == 552) {
		return (((V << 5) & 0x20) | /* A18 */
		    ((V << 3) & 0x10) |     /* A17 */
		    ((V << 1) & 0x08) |     /* A16 */
		    ((V >> 1) & 0x04) |     /* A15 */
		    ((V >> 3) & 0x02) |     /* A14 */
		    ((V >> 5) & 0x01));     /* A13 */
	}
	return V >> 2;
}

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);

	setprg8(0x8000, GetPRGBank(m082.prg[0]));
	setprg8(0xA000, GetPRGBank(m082.prg[1]));
	setprg8(0xC000, GetPRGBank(m082.prg[2]));
	setprg8(0xE000, ~0);
}

static void SyncCHR(void) {
	uint16_t swap = ((m082.ctrl & 2) << 11);

	setchr2(0x0000 ^ swap, m082.chr[0] >> 1);
	setchr2(0x0800 ^ swap, m082.chr[1] >> 1);
	setchr1(0x1000 ^ swap, m082.chr[2]);
	setchr1(0x1400 ^ swap, m082.chr[3]);
	setchr1(0x1800 ^ swap, m082.chr[4]);
	setchr1(0x1C00 ^ swap, m082.chr[5]);
}

static void SyncMirror(void) {
	setmirror(m082.ctrl & 0x01);
}

static DECLFR(ReadWRAM) {
	if (((A >= 0x6000) && (A <= 0x67FF) && (m082.protect[0] == 0xCA)) ||
	    ((A >= 0x6800) && (A <= 0x6FFF) && (m082.protect[1] == 0x69)) ||
	    ((A >= 0x7000) && (A <= 0x73FF) && (m082.protect[2] == 0x84))) {
		return CartBR(A);
	}
	return cpu.openbus;
}

static DECLFW(WriteWRAM) {
	if (((A >= 0x6000) && (A <= 0x67FF) && (m082.protect[0] == 0xCA)) ||
	    ((A >= 0x6800) && (A <= 0x6FFF) && (m082.protect[1] == 0x69)) ||
	    ((A >= 0x7000) && (A <= 0x73FF) && (m082.protect[2] == 0x84))) {
		CartBW(A, V);
	}
}

static DECLFW(WriteReg) {
	switch (A) {
	case 0x7EF0:
	case 0x7EF1:
	case 0x7EF2:
	case 0x7EF3:
	case 0x7EF4:
	case 0x7EF5:
		m082.chr[A & 0x07] = V;
		SyncCHR();
		break;
	case 0x7EF6:
		m082.ctrl = V;
		SyncCHR();
		SyncMirror();
		break;
	case 0x7EF7:
	case 0x7EF8:
	case 0x7EF9:
		m082.protect[A - 0x7EF7] = V;
		break;
	case 0x7EFA:
	case 0x7EFB:
	case 0x7EFC:
		m082.prg[A - 0x7EFA] = V;
		SyncPRG();
		break;
	default:
		/* IRQ emulation ignored since no commercial games uses it */
		break;
	}
}

static void Power(void) {
	memset(&m082, 0, sizeof(m082));

	m082.prg[0] = 0x00;
	m082.prg[1] = 0x01;
	m082.prg[2] = 0xFE;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetReadHandler(0x6000, 0x73FF, ReadWRAM);
	SetWriteHandler(0x6000, 0x73FF, WriteWRAM);
	SetWriteHandler(0x7EF0, 0x7EFF, WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper082_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}

void Mapper552_Init(CartInfo *info) {
	Mapper082_Init(info);
}
