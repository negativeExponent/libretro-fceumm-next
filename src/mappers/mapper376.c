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
} m376;

static SFORMAT StateRegs[] = {
	{ m376.reg, 2, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = ((m376.reg[1] << 4) & 0x10) | ((m376.reg[0] >> 3) & 0x08) | (m376.reg[0] & 0x07);
	uint16_t mask = 0x0F;

	if (m376.reg[0] & 0x80) {
		if (m376.reg[0] & 0x20) {
			setprg32(0x8000, base >> 1);
		} else {
			setprg16(0x8000, base);
			setprg16(0xC000, base);
		}
	} else {
		base <<= 1;
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = ((m376.reg[1] << 8) & 0x100) | ((m376.reg[0] << 1) & 0x80);
	uint16_t mask = 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m376.reg[A & 0x01] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Power(void) {
	memset(&m376, 0, sizeof(m376));
	MMC3_Power();
	SetWriteHandler(0x7000, 0x7FFF, WriteReg);
}

void Mapper376_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
