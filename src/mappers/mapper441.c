/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* 841026C and 850335C multicart circuit boards */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m441;

static SFORMAT StateRegs[] = {
	{ &m441.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m441.reg & 0x08) ? 0x0F : 0x1F;
	uint16_t base = m441.reg << 4;

	if (m441.reg & 0x04) {
		if (!(A & 0x4000)) {
			setprg8(A, (base & ~mask) | ((V & mask) & 0xFD));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) | 0x02));
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m441.reg & 0x40) ? 0x7F : 0xFF;
	uint16_t base = (m441.reg << 3) & 0x180;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		if (!(m441.reg & 0x80)) {
			m441.reg = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		} else {
			CartBW(A, V);
		}
	}
}

static void Reset(void) {
	m441.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m441.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper441_Init(CartInfo *info) {
	int ws = (info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : (info->battery ? 8192 : 0)) / 1024;

	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
