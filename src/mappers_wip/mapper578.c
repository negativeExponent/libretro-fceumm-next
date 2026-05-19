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
#include "mmc3.h"

static struct {
	uint8_t reg;
} m578;

static SFORMAT StateRegs[] = {
	{ &m578.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (((m578.reg & 0x02) && (m578.reg & 0x04)) ? 0x1F : 0x0F);
	uint16_t base = m578.reg << 4;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (((m578.reg & 0x02) && (m578.reg & 0x04)) ? 0xFF : 0x7F);
	uint16_t base = m578.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m578.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static DECLFW(WriteMMC3) {
	if (A & 0x02) {
		WriteReg(A, V);
	} else {
		MMC3_Write(A, V);
	}
}

static void Reset(void) {
	m578.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m578.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper578_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
