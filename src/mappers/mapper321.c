/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2024 negativeExponent
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

/* NC3000M PCB */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8 reg;
} m321;

static SFORMAT StateRegs[] = {
	{ &m321.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRGBank(uint16 A, uint16 V) {
	uint16 mask = 0x0F;
	uint16 base = m321.reg << 2;

	if (m321.reg & 0x08) { /* NROM */
		setprg32(0x8000, (m321.reg & 0x04) | ((m321.reg >> 4) & 0x03));
	} else { /*  MMC3 */
		setprg8(A, ((m321.reg << 2) & ~0x0F) | (V & 0x0F));
	}
}

static void SetCHRBank(uint16 A, uint16 V) {
	setchr1(A, ((m321.reg << 5) & ~0x7F) | (V & 0x7F));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		CartBW(A, V);
		m321.reg = V & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void M321Reset(void) {
	m321.reg = 0;
	MMC3_Reset();
}

static void M321Power(void) {
	m321.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper321_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank;
	MMC3_pwrap = SetPRGBank;
	info->Power = M321Power;
	info->Reset = M321Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
