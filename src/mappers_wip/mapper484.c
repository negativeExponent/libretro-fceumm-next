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
} m484;

static SFORMAT StateRegs[] = {
	{ &m484.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((m484.reg & 0x80) ? 0x1F : 0x3F);
	uint16_t base = ((m484.reg << 5) & 0x20);

	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m484.reg & 0x80)) {
		m484.reg = V;
		MMC3_SyncPRG();
	}
}

static void Power(void) {
	m484.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper484_Init(CartInfo *info) {
	uint16_t ws = 8;
	if (info->iNES2) {
		ws = (info->PRGRamSize + info->PRGRamSaveSize);
		ws /= 1024;
	}
	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
