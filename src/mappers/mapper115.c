/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
	uint8_t reg[4];
} m115;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m115.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = ((m115.reg[0] >> 2) & 0x10) | (m115.reg[0] & 0x0F);
	uint16_t mask = 0x1F;

	if (m115.reg[0] & 0x80) {
		if (m115.reg[0] & 0x20) {
			setprg32(0x8000, base >> 1);
		} else {
			setprg16(0x8000, base);
			setprg16(0xC000, base);
		}
	} else {
		setprg8(A, ((base << 1) & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (m115.reg[1] << 8) | V);
}

static DECLFR(ReadDIP) {
	uint8_t ret = cpu.openbus;

	if ((A & 0x03) == 0x02) {
		return ((ret & ~0x07) | (dipsw & 0x07));
	}
	return ret;
}

static DECLFW(WriteReg) {
	m115.reg[A & 0x03] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m115, 0, sizeof(m115));
	dipsw++;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	memset(&m115, 0, sizeof(m115));
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper115_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}

void Mapper248_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
