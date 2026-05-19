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

/* NES 2.0 Mapper 323 - UNIF FARID_SLROM_8-IN-1 */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m323;

static SFORMAT StateRegs[] = {
	{ &m323.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint8_t mask = 0x07;
	uint8_t base = m323.reg >> 1;

	setprg16(A, (base & ~mask) | (V & mask));
}

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t mask = 0x1F;
	uint16_t base = m323.reg << 1;

	setchr4(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(mmc1.reg[3] & 0x10) && !(m323.reg & 0x08)) {
		m323.reg = V;
		MMC1_SyncCHR();
		MMC1_SyncPRG();
		MMC1_SyncMirror();
	}
}

static void Power(void) {
	memset(&m323, 0, sizeof(m323));
	MMC1_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void Reset(void) {
	memset(&m323, 0, sizeof(m323));
	MMC1_Reset();
}

void Mapper323_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 0, 0);
	MMC1_cwrap = SetCHRBank;
	MMC1_pwrap = SetPRGBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
