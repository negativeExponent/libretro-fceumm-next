/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2025 negativeExponent
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
	uint8 reg;
} m456;

static SFORMAT StateRegs[] = {
	{ &m456.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16 A, uint16 V) {
	uint16 mask = 0x0F;
	uint16 base = m456.reg << 4;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16 A, uint16 V) {
	uint16 mask = 0x7F;
	uint16 base = m456.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m456.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m456, 0, sizeof(m456));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m456, 0, sizeof(m456));
	MMC3_Power();
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
}

void Mapper456_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
