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

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m499;

static SFORMAT StateRegs[] = {
	{ &m499.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = m499.reg << 3;
	uint16_t mask = 0x07;

	setprg16(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = m499.reg << 5;
	uint16_t mask = 0x1F;

	setchr4(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m499.reg & 0x08)) {
		m499.reg = A & 0xFF;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
	}
}

static void Reset(void) {
	m499.reg = 0;
	MMC1_Reset();
}

static void Power(void) {
	m499.reg = 0;
	MMC1_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper499_Init(CartInfo *info) {
	MMC1_Init(info, MMC1A, 0, 0);
	MMC1_pwrap = SetPRG;
	MMC1_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
