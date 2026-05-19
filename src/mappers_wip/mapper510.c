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
	uint8_t dipsw;
} m510;

static SFORMAT StateRegs[] = {
	{ &m510.reg, 1, "EXPR" },
	{ &m510.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m510.reg & 0x20) ? 0x0F : 0x1F;
	uint16_t base = ((m510.reg & 0x08) ? 0x020 : 0x000) | ((m510.reg & 0x40) ? 0x010 : 0x000);

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m510.reg & 0x01) ? 0xFF : 0x7F;
	uint16_t base = ((m510.reg & 0x04) ? 0x100 : 0x000) | ((m510.reg & 0x02) ? 0x080 : 0x000);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if (m510.reg == 0) {
		m510.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m510.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m510.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper510_Init(CartInfo *info) {
	int ws = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192;
	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
