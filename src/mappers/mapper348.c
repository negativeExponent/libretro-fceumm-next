/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2008 CaH4e3
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

/* NES 2.0 Mapper 348 is used for multicarts using 830118C-numbered PCBs such as
 * 1994 New Series Red Pig 7-in-1. Its UNIF board name is BMC-830118C. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m348;

static SFORMAT StateRegs[] = {
	{ &m348.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m348.reg << 2;
	uint16_t bank;

	if ((m348.reg & 0x0C) == 0x0C) { /* GNROM-like */
		if (!(A & 0x4000)) {
			setprg8(A, (base & ~mask) | ((V & mask) & 0xFD));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) | 0x02));
		}
	} else {
		bank = (base & ~mask) | (V & mask);
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x7F;
	uint16_t base = m348.reg << 5;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m348.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m348.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m348.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper348_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
