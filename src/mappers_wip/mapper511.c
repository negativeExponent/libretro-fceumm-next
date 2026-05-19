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
} m511;

static SFORMAT StateRegs[] = {
	{ &m511.reg, 1, "EXPR" },
	{ &m511.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m511.reg << 4;

	if (m511.reg & 0x04) {
		if (m511.reg & 0x02) {
			setprg8(0x8000, (base & ~mask) | (MMC3_GetPRGBank(0) & 0xFE) | 0);
			setprg8(0xA000, (base & ~mask) | (MMC3_GetPRGBank(1) & 0xFE) | 1);
			setprg8(0xC000, (base & ~mask) | (MMC3_GetPRGBank(0) & 0xFE) | 0);
			setprg8(0xE000, (base & ~mask) | (MMC3_GetPRGBank(1) & 0xFE) | 1);
		} else {
			setprg8(0x8000, (base & ~mask) | (MMC3_GetPRGBank(0) & 0xFC) | 0);
			setprg8(0xA000, (base & ~mask) | (MMC3_GetPRGBank(1) & 0xFC) | 1);
			setprg8(0xC000, (base & ~mask) | (MMC3_GetPRGBank(0) & 0xFC) | 2);
			setprg8(0xE000, (base & ~mask) | (MMC3_GetPRGBank(1) & 0xFC) | 3);
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = m511.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if (m511.reg & 0x08) {
		A = (A & ~0x03) | (m511.dipsw & 0x03);
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m511.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m511.reg = 0;
	m511.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m511.reg = 0;
	m511.dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper511_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
