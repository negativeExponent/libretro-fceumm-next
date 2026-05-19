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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* NES 2.0 Mapper 473 denotes the KJ01A-18 MMC3-compatible multicart PCB, used
 * by a Coolbaby 600-in-1 multicart.
 * 114-in-1 (KJ01A18) (Unl)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
} m473;

static SFORMAT StateRegs[] = {
	{ m473.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint32_t mask = (m473.reg[0] & 0x20) ? (0xFF >> (0x07 - (m473.reg[0] & 0x07))) : 0x00;
	uint32_t base = (m473.reg[0] & 0x20) ? (m473.reg[1] | (m473.reg[2] << 8)) : 0x3F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (!(m473.reg[0] & 0x80)) {
		setchr8(m473.reg[3]);
	} else {
		setchr1(A, V);
	}
}

static DECLFW(WriteReg) {
	m473.reg[A & 0x03] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m473, 0, sizeof(m473));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m473, 0, sizeof(m473));
	MMC3_Power();
	SetWriteHandler(0x4800, 0x4FFF, WriteReg);
}

void Mapper473_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
