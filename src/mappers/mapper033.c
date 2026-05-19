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
 */

/* iNES Mapper 33 - Taito TC0190/TC0350 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t prg[2], chr[6], mirror;
} m033;

static SFORMAT StateRegs[] = {
	{ m033.prg, 2, "PREG" },
	{ m033.chr, 6, "CREG" },
	{ &m033.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m033.prg[0]);
	setprg8(0xA000, m033.prg[1]);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr2(0x0000, m033.chr[0]);
	setchr2(0x0800, m033.chr[1]);
	setchr1(0x1000, m033.chr[2]);
	setchr1(0x1400, m033.chr[3]);
	setchr1(0x1800, m033.chr[4]);
	setchr1(0x1C00, m033.chr[5]);
}

static void SyncMirror(void) {
	setmirror(((m033.mirror >> 6) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	switch (A & 0xE003) {
	case 0x8000:
	case 0x8001:
		m033.prg[A & 0x01] = V;
		m033.mirror = m033.prg[0] & 0x40;
		SyncPRG();
		SyncMirror();
		break;
	case 0x8002:
	case 0x8003:
		m033.chr[A & 0x01] = V;
		SyncCHR();
		break;
	case 0xA000:
	case 0xA001:
	case 0xA002:
	case 0xA003:
		m033.chr[2 + (A & 0x03)] = V;
		SyncCHR();
		break;
	}
}

static void Power(void) {
	memset (&m033, 0, sizeof(m033));

	m033.prg[0] = 0x00;
	m033.prg[1] = 0x01;

	m033.chr[0] = 0x00;
	m033.chr[1] = 0x01;
	m033.chr[2] = 0x04;
	m033.chr[3] = 0x05;
	m033.chr[4] = 0x06;
	m033.chr[5] = 0x07;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper033_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
