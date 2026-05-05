/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
} m618;

static SFORMAT StateRegs[] = {
	{ &m618.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m618.reg >> 1 & 4;

	if (m618.reg & 0x02) {
		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		setprg32(0x8000,
		         ((m618.reg >> 3) & 0x08) | (m618.reg & 0x04) | ((m618.reg >> 4) & 0x01) |
		             ((m618.reg << 1) & 0x02));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m618.reg & 0x80) {
		uint8_t bank = A >> 10;
		setchr1(A, 0x200 | ((MMC3_GetCHRBank((bank & 0x04) | ((bank >> 1) & 0x01)) << 1) | (bank & 1)));
	} else {
		setchr1(A, ((m618.reg << 6) & 0x100) | (V & 0xFF));
	}
}

static DECLFW(WriteReg) {
	m618.reg = A & 0xFF; 
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static void Reset(void) {
	m618.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m618.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper618_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
}
