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

/* NES 2.0 Mapper 344
 * BMC-GN-26
 * Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 (3-in-1,6-in-1,Unl)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m344;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m344.reg, 1, "EXPR" },
	{ 0 } 
};

static uint8_t prg_bank_order[2][4] = {
	{ 0, 1, 2, 3 }, /* normal bank order */
	{ 0, 3, 1, 2 } /* wrong bank order, added for compatibility */
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = prg_bank_order[(iNESCart.PRGCRC32 == 0xAB2ACA46)][m344.reg & 0x03] << 4;
	uint16_t mask = 0x0F;

	if (m344.reg & 0x04) { /* NROM-256 */
		V = (MMC3_GetPRGBank(0) & ~0x03) | ((A >> 13) & 0x03);
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m344.reg & 0x02) ? 0x7F : 0xFF;
	uint16_t base = (m344.reg & 0x03) << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if (m344.reg & 0x08) {
		return dipsw ? 0x78 : 0;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m344.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 6:
		case 7:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_SyncPRG();
			break;
		default:
			MMC3_Write(A, V);
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Reset(void) {
	memset(&m344, 0, sizeof(m344));
	dipsw = (dipsw + 1) & 0x01;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m344, 0, sizeof(m344));
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper344_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
