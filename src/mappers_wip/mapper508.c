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
} m508;

static SFORMAT StateRegs[] = {
	{ &m508.reg, 1, "EXPR" },
	{ &m508.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m508.reg & 0x80) {
		uint16_t mask = 0x1F;
		uint16_t base = (m508.reg >> 1);

		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		uint16_t bank = (((m508.reg >> 2) & ~0x0F) | (m508.reg & 0x0F));
		if (m508.reg & 0x20) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr8(0);
}

static DECLFR(ReadDIP) {
	return m508.dipsw;
}

static DECLFW(WriteReg) {
	m508.reg = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m508.reg = 0x0F;
	m508.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m508.reg = 0x0F;
	m508.dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper508_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
