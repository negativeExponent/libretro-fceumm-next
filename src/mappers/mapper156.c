/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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
 * DIS23C01 DAOU ROM CONTROLLER, Korea
 * Metal Force (K)
 * Buzz and Waldog (K)
 * General's Son (K)
 *
 */

#include "mapinc.h"

static struct {
	uint8_t prg, mirror;
	uint16_t chr[8];
} m156;

static SFORMAT StateRegs[] = {
	{ &m156.chr[0], 2 | FCEUSTATE_RLSB, "CRE0" },
	{ &m156.chr[1], 2 | FCEUSTATE_RLSB, "CRE1" },
	{ &m156.chr[2], 2 | FCEUSTATE_RLSB, "CRE2" },
	{ &m156.chr[3], 2 | FCEUSTATE_RLSB, "CRE3" },
	{ &m156.chr[4], 2 | FCEUSTATE_RLSB, "CRE4" },
	{ &m156.chr[5], 2 | FCEUSTATE_RLSB, "CRE5" },
	{ &m156.chr[6], 2 | FCEUSTATE_RLSB, "CRE6" },
	{ &m156.chr[7], 2 | FCEUSTATE_RLSB, "CRE7" },
	{ &m156.prg, 1, "PREG" },
	{ &m156.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, m156.prg);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr1(0x0000, m156.chr[0]);
	setchr1(0x0400, m156.chr[1]);
	setchr1(0x0800, m156.chr[2]);
	setchr1(0x0C00, m156.chr[3]);
	setchr1(0x1000, m156.chr[4]);
	setchr1(0x1400, m156.chr[5]);
	setchr1(0x1800, m156.chr[6]);
	setchr1(0x1C00, m156.chr[7]);
}

static void SyncMirror(void) {
	switch (m156.mirror) {
	case 0:
		setmirror(MI_V);
		break;
	case 1:
		setmirror(MI_H);
		break;
	default:
		setmirror(MI_0);
		break;
	}
}

static DECLFW(WriteReg) {
	switch (A & 0xCFFC) {
	case 0xC000:
	case 0xC004:
	case 0xC008:
	case 0xC00C: {
		uint8_t index = ((A >> 1) & 0x04) | (A & 0x03); /* [0–3] + 0x04 if A is 0xC008 or 0xC00C */
		uint16_t mask = (A & 0x04) ? 0x00FF : 0xFF00;
		uint16_t value = (A & 0x04) ? (V << 8) : V;

		m156.chr[index] = (m156.chr[index] & mask) | value;
		SyncCHR();
		break;
	}
	case 0xC010:
		m156.prg = V;
		SyncPRG();
		break;
	case 0xC014:
		m156.mirror = V;
		SyncMirror();
		break;
	}
}

static void Reset(void) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m156, 0, sizeof(m156));

	m156.mirror = MI_0;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0xC000, 0xFFFF, WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void Close(void) {
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper156_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
}
