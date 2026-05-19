/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2020
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 267 - 8-in-1 JY-119 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m267;

static SFORMAT StateRegs[] = {
	{ &m267.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint16_t base = ((m267.reg & 0x20) >> 2) | (m267.reg & 0x06);

	setprg8(A, (base << 4) | (V & 0x1F));
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t base = ((m267.reg & 0x20) >> 2) | (m267.reg & 0x06);

	setchr1(A, (base << 6) | (V & 0x7F));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		if (!(m267.reg & 0x80)) {
			m267.reg = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static void Reset(void) {
	m267.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m267.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper267_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank;
	MMC3_pwrap = SetPRGBank;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
