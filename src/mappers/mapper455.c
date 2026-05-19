/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
} m455;

static SFORMAT StateRegs[] = {
	{ m455.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m455.reg[1] & 0x01) ? 0x1F : 0x0F;
	uint16_t base = ((m455.reg[0] >> 2) & 0x10) | ((m455.reg[1] << 1) & 0x08) | ((m455.reg[0] >> 2) & 0x07);

	if (m455.reg[0] & 0x01) {
		if (m455.reg[0] & 0x02) {
			setprg32(0x8000, base >> 1);
		} else {
			setprg16(0x8000, base);
			setprg16(0xC000, base);
		}
	} else {
		base = (base << 1);
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m455.reg[1] & 0x02) ? 0xFF : 0x7F;
	uint16_t base = ((m455.reg[0] >> 2) & 0x10) | ((m455.reg[1] << 1) & 0x08) | ((m455.reg[0] >> 2) & 0x07);

	base = (base << 4);
	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m455.reg[0] = V;
		m455.reg[1] = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m455, 0, sizeof(m455));
	m455.reg[0] = 1;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m455, 0, sizeof(m455));
	m455.reg[0] = 1;
	MMC3_Power();
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
}

void Mapper455_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
