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
} m568;

static SFORMAT StateRegs[] = {
	{ &m568.reg, 1, "EXPR" },
	{ &m568.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((m568.reg & 0x20) ? 0x1F : 0x0F);
	uint16_t base = (m568.reg << 1) & ~mask;
	uint16_t bank = (A >> 13) & 0x03;

	if (m568.reg & 0x02) {
		if (m568.reg & 0x01) {
			base = base | (MMC3_GetPRGBank(0) & mask);
			mask = 0x03;
			V = bank;
		} else {
			base = base | (MMC3_GetPRGBank(0) & mask);
			mask = 0x01;
			V = bank & 0x01;
		}
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = m568.reg & 0x20 ? 0xFF : 0x7F;
	uint16_t base = m568.reg << 4;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if (m568.reg & 0x40) {
		A = ((A & ~0x0F) | (m568.dipsw & 0x0F));
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m568.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
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
	m568.reg = 0;
	m568.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m568.reg = 0;
	m568.dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper568_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
