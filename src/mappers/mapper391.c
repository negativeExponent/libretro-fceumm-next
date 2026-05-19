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

/* BS-110 PCB, previously called NC7000M due to a mix-up. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m391;

static SFORMAT StateRegs[] = {
	{ m391.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = (m391.reg[0] & 0x08) ? 0x0F : 0x1F;
	uint8_t base = (m391.reg[0] << 4) & 0x30;

	if (m391.reg[0] & 0x20) {
		if (!(A & 0x4000)) { /* GNROM */
			uint8_t A14 = (m391.reg[0] >> 1) & 0x02;

			setprg8(A, (base & ~mask) | ((V & mask) & ~A14));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) |  A14));
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m391.reg[0] & 0x40) ? 0x7F : 0xFF;
	uint16_t base = ((m391.reg[0] << 3) & 0x80) | ((m391.reg[1] << 8) & 0x100);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		if (!(m391.reg[0] & 0x80)) {
			m391.reg[0] = V;
			m391.reg[1] = ((A >> 8) & 0xFF);
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static void Reset(void) {
	m391.reg[0] = m391.reg[1] = 0;
	MMC3_Reset();
}

static void Power(void) {
	m391.reg[0] = m391.reg[1] = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper391_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
