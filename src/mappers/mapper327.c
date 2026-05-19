/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *	Copyright (C) 2015 Cluster
 *	http://clusterrr.com
 *	clusterrr@clusterrr.com
 *
 *  Copyright (C) 2023-2024-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301	USA
 */

/*
	NES 2.0 mapper 327 is used for a 6-in-1 multicart.
	Its UNIF board name is BMC-10-24-C-A1.

	MMC3-based multicart mapper with CHR RAM, CHR ROM and PRG RAM

	$6000-7FFF:	A~[011xxxxx xxMRSBBB]	Multicart m327.reg
		This register can only be written to if PRG-RAM is enabled and writable (see $A001)
		and BBB = 000 (power on state)

	BBB = CHR+PRG block select bits (A19, A18, A17 for both PRG and CHR)
	S = PRG block size (0=256k	 1=128k)
	R = CHR mode (0=CHR ROM	 1=CHR RAM)
	M = CHR block size (0=256k	 1=128k)
		ignored when S is 0 for some reason

 Example Game:
 --------------------------
 6 in 1 multicart (SMB3, TMNT2, Contra, Ninja Cat, Ninja Crusaders, Rainbow Islands 2)
*/

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m327;

static SFORMAT StateRegs[] = {
	{ &m327.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint8_t base = (m327.reg << 4) & 0x70;
	uint8_t mask = (m327.reg & 0x08) ? 0x1F : 0x0F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	if (m327.reg & 0x10) {
		setchr8r(0x10, 0);
	} else {
		uint16_t base = (m327.reg << 7) & 0x380;
		uint16_t mask = (m327.reg & 0x20) ? 0xFF : 0x7F;

		setchr1(A, base | (V & mask));
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		CartBW(A, V);
		if ((m327.reg & 0x07) == 0) {
			m327.reg = A & 0x3F;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static void Reset(void) {
	m327.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m327.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper327_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_pwrap = SetPRGBank;
	MMC3_cwrap = SetCHRBank;

	info->Power = Power;
	info->Reset = Reset;

	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
