/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 * NTDEC, ASDER games
 *
 */

#include "mapinc.h"

static struct {
	uint8_t reg[8];
	uint8_t mirror, cmd, chrBase;
} m112;

static SFORMAT StateRegs[] = {
	{ &m112.cmd, 1, "CMD" },
	{ &m112.mirror, 1, "MIRR" },
	{ &m112.chrBase, 1, "CHRB" },
	{ m112.reg, 8, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8r(0x10, 0x6000, 0);

	setprg8(0x8000, m112.reg[0]);
	setprg8(0xA000, m112.reg[1]);
	setprg16(0xC000, ~0);	
}

static void SyncCHR(void) {
	setchr2(0x0000, (m112.reg[2] >> 1));
	setchr2(0x0800, (m112.reg[3] >> 1));
	setchr1(0x1000, ((m112.chrBase << 4) & 0x100) | m112.reg[4]);
	setchr1(0x1400, ((m112.chrBase << 3) & 0x100) | m112.reg[5]);
	setchr1(0x1800, ((m112.chrBase << 2) & 0x100) | m112.reg[6]);
	setchr1(0x1C00, ((m112.chrBase << 1) & 0x100) | m112.reg[7]);
}

static void SyncMirror(void) {
	setmirror((m112.mirror & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0xE000) {
	case 0x8000:
		m112.cmd = V;
		break;
	case 0xA000:
		m112.reg[m112.cmd & 0x07] = V;
		SyncPRG();
		SyncCHR();
		break;
	case 0xC000:
		m112.chrBase = V;
		SyncCHR();
		break;
	case 0xE000:
		m112.mirror = V;
		SyncMirror();
		break;
	}
}

static void Close(void) {
}

static void Power(void) {
	memset(&m112, 0, sizeof(m112));

	m112.reg[0] = 0;
	m112.reg[1] = 1;
	m112.reg[2] = 0;
	m112.reg[3] = 2;
	m112.reg[4] = 4;
	m112.reg[5] = 5;
	m112.reg[6] = 6;
	m112.reg[7] = 7;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);

	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper112_Init(CartInfo *info) {
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : (info->battery ? 8192 : 0);
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(8192);
		SetupCartPRGMapping(0x10, WRAM, 8192, 1);
		AddExState(WRAM, 8192, 0, "WRAM");
	}
}
