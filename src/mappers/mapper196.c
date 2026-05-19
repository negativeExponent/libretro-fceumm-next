/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * iNES Mapper 196 denotes circuit boards by MRCM, who mostly made Mario-themed
 * hacks of existing games. For copy protection purposes, the MMC3 clone's A0
 * input is connected to A1 or A2 instead. Additonally, one variant uses mapper
 * 189-style PRG banking. Submappers denote the particular variant used:
 *
 * Submapper 0: Actual variant unknown; use the following heuristics:
 *  MMC3's CPU A0 input =1 if (CPU A1=1 or CPU A2=1 or CPU A3=1) and CPU A0=0
 *  Use normal MMC3 PRG banking until CPU 6000-7FFF is written to, after which
 *  use mapper 189-style PRG banking
 * Submapper 1: MMC3 CPU A0 input=CPU A1, normal MMC3 PRG banking
 * Submapper 2: MMC3 CPU A0 input=CPU A2, normal MMC3 PRG banking
 * Submapper 3: MMC3 CPU A0 input=CPU A1, mapper 189-style PRG banking.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m196;

static SFORMAT StateRegs[] = {
	{ m196.reg, 2, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m196.reg[0]) {
		setprg32(0x8000, m196.reg[1] >> 4);
	} else {
		setprg8(A, V);
	}
}

static DECLFW(WriteNROM) {
	m196.reg[0] = 1;
	m196.reg[1] = V;
	MMC3_SyncPRG();
}

static DECLFW(WriteASIC) {
	uint8_t A0;
	switch (iNESCart.submapper) {
	case 1:
		A0 = (A >> 1) & 0x01;
		break;
	case 2:
		A0 = (A >> 2) & 0x01;
		break;
	case 3:
		A0 = (A >> 1) & 0x01;
		break;
	default:
		A0 = (!!(A & 0x0E) ^ (A & 0x01));
		break;
	}
	A = (A & 0xF000) | A0;
	MMC3_Write(A, V);
}

static void Power(void) {
	memset(&m196, 0, sizeof(m196));
	MMC3_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);
	if ((iNESCart.submapper == 0) || (iNESCart.submapper == 3)) {
		SetWriteHandler(0x6000, 0x7FFF, WriteNROM);
	}
}

void Mapper196_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
