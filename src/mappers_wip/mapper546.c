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
} m546;

static SFORMAT StateRegs[] = {
	{ &m546.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m546.reg & 0x10) {
		setprg16(A, 0x10 | (V & 0x0F));
	} else {
		if (m546.reg & 0x20) {
			setprg32(0x8000, m546.reg >> 1);
		} else {
			setprg16(0x8000, m546.reg);
			setprg16(0xC000, m546.reg);
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint8_t writable = (m546.reg & 0x80) ? FALSE : TRUE;
	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], writable);
	setchr8(0);
}

static DECLFW(WriteReg) {
	if (!(A & 0x0F00)) {
		m546.reg = A & 0xFF;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
	}
	MMC1_Write(A, V);
}

static void Reset(void) {
	m546.reg = 0;
	MMC1_Reset();
}

static void Power(void) {
	m546.reg = 0;
	MMC1_Power();
	SetWriteHandler(0x8000, 0x9FFF, WriteReg);
}

void Mapper546_Init(CartInfo *info) {
	int ws = (info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192) / 1024;
	int bs = (info->battery ? ws : 0);
	MMC1_Init(info, MMC1B, ws, bs);
	MMC1_pwrap = SetPRG;
	MMC1_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
