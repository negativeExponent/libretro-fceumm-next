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
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m393;

static SFORMAT StateRegs[] = {
	{ m393.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m393.reg[0] & 0x20) {
		if (m393.reg[0] & 0x10) { /* UNROM */
			setprg16(0x8000, (m393.reg[0] << 3) | (m393.reg[1] & 0x07));
			setprg16(0xC000, (m393.reg[0] << 3) | 0x07);
		} else { /* BNROM */
			setprg32(0x8000, (m393.reg[0] << 2) | ((mmc3.reg[6] >> 2) & 0x03));
		}
	} else {
		setprg8(A, (m393.reg[0] << 4) | (V & 0x0F));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m393.reg[0] & 0x08) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, (m393.reg[0] << 8) | (V & 0xFF));
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m393.reg[0] = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteMMC3) {
	m393.reg[1] = V;
	if (m393.reg[0] & 0x20) {
		MMC3_SyncPRG();
	}
	MMC3_Write(A, V);
}

static void Power(void) {
	m393.reg[0] = m393.reg[1] = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

static void Reset(void) {
	m393.reg[0] = m393.reg[1] = 0;
	MMC3_Reset();
}

void Mapper393_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
