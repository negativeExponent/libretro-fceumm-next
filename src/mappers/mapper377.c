/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 * Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 377 - NES 2.0 Mapper 377 is used for the
 * 1998 Super Game 8-in-1 (JY-111) pirate multicart. It works similarly to Mapper 267 except it has an outer 256KiB
 * PRG-ROM bank.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m377;

static SFORMAT StateRegs[] = {
	{ &m377.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = (((m377.reg & 0x20) >> 2) | (m377.reg & 0x06)) << 3;
	uint16_t mask = 0x0F;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = (((m377.reg & 0x20) >> 2) | (m377.reg & 0x06)) << 6;
	uint16_t mask = 0x7F;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m377.reg & 0x80)) {
		m377.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m377, 0, sizeof(m377));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m377, 0, sizeof(m377));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper377_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
