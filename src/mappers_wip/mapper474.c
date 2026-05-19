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

/* NTDEC N625231*/

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m474;

static SFORMAT StateRegs[] = {
	{ &m474.reg, 1, "EXPR" },
	{ 0 }
};

static void SyncPRG(void) {
	uint16_t base = (m474.reg << 5) & 0x20;
	uint16_t bank = base | (m474.reg >> 3) & 0x1F;

	if (m474.reg & 0x04) {
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else {
		setprg32(0x8000, bank >> 1);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, ((m474.reg << 7) & 0x100) | V);
}

static DECLFW(WriteExtra) {
	if (A & 0x100) {
		m474.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m474.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m474.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x4100, 0x5FFF, WriteExtra);
}

void Mapper474_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_SyncPRG = SyncPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
