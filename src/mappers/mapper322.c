/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 322
 * BMC-K-3033
 * 35-in-1 (K-3033)
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_322
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m322;

static SFORMAT StateRegs[] = {
	{ &m322.reg, 1, "EXPR" },
	{ 0 }
};

static uint16_t GetOuterBank(void) {
	return ((m322.reg >> 4) & 0x04) | ((m322.reg >> 3) & 0x03);
}

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint16_t base = GetOuterBank() << 4;
	uint16_t mask = (m322.reg & 0x80) ? 0x1F : 0x0F;

	if (!(m322.reg & 0x20)) {
		uint16_t bank = (base >> 1) | (m322.reg & 0x07);

		if (m322.reg & 0x03) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t base = GetOuterBank() << 7;
	uint16_t mask = (m322.reg & 0x80) ? 0xFF : 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m322.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Power(void) {
	m322.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void Reset(void) {
	m322.reg = 0;
	MMC3_Reset();
}

void Mapper322_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRGBank;
	MMC3_cwrap = SetCHRBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
