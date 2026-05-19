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
} m569;

static SFORMAT StateRegs[] = {
	{ &m569.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = (m569.reg << 4) & ~mask;
	uint16_t bank = (A >> 13) & 0x03;

	if (m569.reg & 0x08) {
		base = base | (MMC3_GetPRGBank(0) & mask);
		mask = 0x03;
		V = bank;
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = ((m569.reg & 0x04) ? 0x1FF : ((m569.reg & 0x02) ? 0xFF : 0x7F));
	uint16_t base = (m569.reg << 7) & ~mask;
	uint16_t bank = (A >> 10) & 0x07;

	if (m569.reg & 0x04) {
		base = base | ((MMC3_GetCHRBank((bank & 0x06) | ((bank >> 1) & 0x01)) << 1) & mask);
		mask = 0x01;
		V = bank;
	}

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m569.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		mmc3.reg[mmc3.cmd & 0x07] = V;
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			MMC3_SyncCHR();
			break;
		case 6:
		case 7:
			MMC3_SyncPRG();
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Reset(void) {
	m569.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m569.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper569_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
