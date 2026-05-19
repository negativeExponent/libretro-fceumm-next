/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * SL1632 2-in-1 protected board, similar to SL12
 * Samurai Spirits Rex (Full)
 *
 */

#include "mapinc.h"
#include "mmc3.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m014;

static SFORMAT StateRegs[] = {
	{ &m014.reg, 1, "REGS" },
	{ 0 }
};

static uint8_t GetChrBase(uint16_t A) {
	if (A & 0x1000) {
		if (A & 0x800) {
			return ((m014.reg & 0x80) >> 7);
		} else {
			return ((m014.reg & 0x20) >> 5);
		}
	}
	return ((m014.reg & 0x08) >> 3);
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = GetChrBase(A) << 8;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SetCHRBank_vrc24(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = GetChrBase(A) << 8;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteASIC) {
	if (A == 0xA131) {
		m014.reg = V;
		if (m014.reg & 0x02) {
			MMC3_SyncCHR();
		} else {
			VRC24_SyncCHR();
		}
	} else {
		if (m014.reg & 0x02) {
			MMC3_Write(A, V);
		} else {
			VRC24_Write(A, V);
		}
	}
}

static void StateRestore(int version) {
	if (m014.reg & 0x02) {
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	} else {
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}

static void Power(void) {
	m014.reg = 0;
	VRC24_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);
}

void Mapper014_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank_mmc3;

	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, FALSE, TRUE);
	VRC24_cwrap = SetCHRBank_vrc24;

	info->Power = Power;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
