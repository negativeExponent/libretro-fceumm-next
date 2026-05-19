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
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m364;

static SFORMAT StateRegs[] = {
	{ &m364.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m364.reg & 0x20) ? 0x0F : 0x1F;
	uint16_t base = (m364.reg >> 1) & 0x20;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m364.reg & 0x20) ? 0x7F : 0xFF;
	uint16_t base = (m364.reg << 4) & 0x100;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m364.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	m364.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper364_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
