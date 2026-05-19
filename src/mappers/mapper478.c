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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* NES 2.0 Mapper 478 */
/* 7-in-1 (AB701) (Unl) */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m478;

static SFORMAT StateRegs[] = {
	{ m478.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = (iNESCart.submapper == 1) ? (m478.reg[0] << 3) : (m478.reg[0] << 2);
	uint16_t mask = (iNESCart.submapper == 1) ? 0x0F : (((m478.reg[0] & 0x0C) == 0x0C) ? 0x03 : 0x0F);

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = (iNESCart.submapper == 1) ? (m478.reg[0] << 6) : (m478.reg[0] << 5);
	uint16_t mask = (iNESCart.submapper == 1) ? 0x7F : (((m478.reg[0] & 0x0C) == 0x0C) ? 0x1F : 0x7F);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		if (m478.reg[1] & 0x60) {
			m478.reg[0] = A & 0xFF;
			m478.reg[1] = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static DECLFW(WriteMMC3) {
	if (m478.reg[1] & 0x80) {
		MMC3_Write(A, V);
	} else {
		/* Mickey Mouse */
		mmc3.reg[0] = (mmc3.reg[0] & ~0x18) | ((V << 3) & 0x18);
		mmc3.reg[1] = (mmc3.reg[1] & ~0x18) | ((V << 3) & 0x18);
		mmc3.reg[2] = (mmc3.reg[2] & ~0x18) | ((V << 3) & 0x18);
		mmc3.reg[3] = (mmc3.reg[3] & ~0x18) | ((V << 3) & 0x18);
		mmc3.reg[4] = (mmc3.reg[4] & ~0x18) | ((V << 3) & 0x18);
		mmc3.reg[5] = (mmc3.reg[5] & ~0x18) | ((V << 3) & 0x18);
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m478, 0, sizeof(m478));
	m478.reg[1] = 0xF0;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m478, 0, sizeof(m478));
	m478.reg[1] = 0xF0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper478_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
