/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* NES 2.0 Mapper 498
 - BMC-K-3011    (34-in-1)
 - BMC-K-3011-25 (29-in-1)
 */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m498;

static SFORMAT StateRegs[] = {
	{ &m498.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x07;
	uint16_t base = m498.reg;

	setprg16(A, (base & ~mask) | (V & mask));
}

static void SyncPRG(void) {
	if (m498.reg & 0x20) {
		MMC1_SyncPRG_default();
	} else {
		uint16_t bank = m498.reg;
		if (m498.reg & 0x04) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x1F;
	uint16_t base = (m498.reg << 2);

	setchr4(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m498.reg = A & 0xFF;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
}

static void Reset(void) {
	m498.reg = 0;
	MMC1_Reset();
}

static void Power(void) {
	m498.reg = 0;
	MMC1_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper498_Init(CartInfo *info) {
	MMC1_Init(info, MMC1A, 0, 0);
	MMC1_pwrap = SetPRG;
	MMC1_cwrap = SetCHR;
	MMC1_SyncPRG = SyncPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
