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
} m545;

static SFORMAT StateRegs[] = {
	{ &m545.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if ((m545.reg & 0x08) && !(V & 0x10)) {
		setprg8(A, (0x40 | (V & 0x0F)));
	} else {
		setprg8(A, ((m545.reg << 4) & ~0x0F) | (V & 0x0F));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (m545.reg << 7) | ((~m545.reg << 4) & 0x40) | (V & 0x7F));
}

static DECLFW(WriteReg) {
	if ((A & 0x020) && (A & 0x100)) {
		m545.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	} else {
		MMC3_Write(A, V);
	}
}

static void Reset(void) {
	m545.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m545.reg = 0;
	MMC3_Power();
	SetWriteHandler(0xF000, 0xFFFF, WriteReg);
}

void Mapper545_Init(CartInfo *info) {
	int ws = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192;
	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
