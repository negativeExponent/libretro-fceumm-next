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

/*
 * - Mapper 56 - UNL KS202
 *   FDS Conversion: Super Mario Bros. 3 (Pirate, Alt)
 *   similar to mapper 142 but use WRAM instead? $D000 additional IRQ trigger
 * - fix IRQ counter, noticeable in status bars of both SMB2J(KS7032) and SMB3J(KS202)
 */

#include "mapinc.h"
#include "ks202.h"

static struct {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t mirror;
} m056;

static SFORMAT StateRegs[] = {
	{ m056.prg, 4, "PREG" },
	{ m056.chr, 8, "CREG" },
	{ &m056.mirror, 1, "MIRR" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, (m056.prg[0] & 0x10) | (ks202.reg[1] & 0x0F));
	setprg8(0xA000, (m056.prg[1] & 0x10) | (ks202.reg[2] & 0x0F));
	setprg8(0xC000, (m056.prg[2] & 0x10) | (ks202.reg[3] & 0x0F));
	setprg8(0xE000, (m056.prg[3] & 0x10) | (~0 & 0x0F));
}

static void SyncCHR(void) {
	setchr1(0x0000, m056.chr[0]);
	setchr1(0x0400, m056.chr[1]);
	setchr1(0x0800, m056.chr[2]);
	setchr1(0x0C00, m056.chr[3]);
	setchr1(0x1000, m056.chr[4]);
	setchr1(0x1400, m056.chr[5]);
	setchr1(0x1800, m056.chr[6]);
	setchr1(0x1C00, m056.chr[7]);
}

static void SyncMirror(void) {
	setmirror(m056.mirror & 0x01);
}

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);

	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static DECLFW(WriteReg) {
	static int tmp = 0;
	switch (A & 0x0C00) {
	case 0x000:
		m056.prg[A & 0x03] = V;
		break;
	case 0x800:
		m056.mirror = V;
		break;
	case 0xC00:
		m056.chr[A & 0x07] = V;
		break;
	}
	ks202.reg[ks202.cmd & 0x07] = V;
	Sync();
}

static void Reset(void) {
	memset(&m056, 0, sizeof(m056));
	m056.prg[0] = 0x10;
	m056.prg[1] = 0x10;
	m056.prg[2] = 0x10;
	m056.prg[3] = 0x10;
	Sync();
}

static void Power(void) {
	memset(&m056, 0, sizeof(m056));
	m056.prg[0] = 0x10;
	m056.prg[1] = 0x10;
	m056.prg[2] = 0x10;
	m056.prg[3] = 0x10;
	KS202_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteReg);
}

void Mapper056_Init(CartInfo *info) {
	KS202_Init(info, Sync, 1, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(&StateRegs, ~0, 0, 0);
}
