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
} m567;

static SFORMAT StateRegs[] = {
	{ &m567.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m567.reg << 4;
	uint16_t bank;

	if (m567.reg == 0x08) {
		bank = (A >> 13) & 0x03;
		setprg8(A, (base & ~mask) | (MMC3_GetPRGBank(bank & 0x01) & 0xFC) | bank);
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t bank, base, mask;

	if (m567.reg == 0x0F) {
		mask = 0x1FF;
		base = 0x400;
		bank = (A >> 10) & 0x07;
		V = MMC3_GetCHRBank((bank & 0x06) | ((bank >> 1) & 0x01)) << 1;
		setchr1(A, (base & ~mask) | (V & mask) | (bank & 0x01));
	} else {
		mask = 0x7F;
		base = (m567.reg << 7) & 0x380;

		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m567.reg = A & 0xFF;
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
	m567.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m567.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper567_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
