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

/* 3927 PCB, Reset-based MMC1/CNROM multicart. The common dump with mapper
 * number 3927 has an unrealistic bank order. */

#include "mapinc.h"
#include "latch.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m483;

static SFORMAT StateRegs[] = {
	{ &m483.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG_slrom(uint16_t A, uint16_t V) {
	setprg16(A, (m483.reg << 3) | (V & 0x07));
}

static void SetCHR_slrom(uint16_t A, uint16_t V) {
	setchr4(A, (m483.reg << 5) | (V & 0x1F));
}

static void SetPRG_serom(uint16_t A, uint16_t V) {
	setprg32(0x8000, 0x0F);
}

static void SetCHR_serom(uint16_t A, uint16_t V) {
	setchr4(A, (0x78 | (V & 0x07)));
}

static void SyncCNROM(void) {
	setprg32(0x8000, (0x0C + m483.reg - 3));
	setchr8(0x30 | ((m483.reg - 3) << 2) | (latch.data & 0x03));
	setmirror(MI_V);
}

static void Sync(void) {
	if (m483.reg <= 2) {
		MMC1_pwrap = SetPRG_slrom;
		MMC1_cwrap = SetCHR_slrom;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncMirror_default();
	} else {
		if (m483.reg <= 5) {
			SyncCNROM();
		} else {
			MMC1_pwrap = SetPRG_serom;
			MMC1_cwrap = SetCHR_serom;
			MMC1_SyncPRG();
			MMC1_SyncCHR();
			MMC1_SyncMirror_default();
		}
	}
}
static DECLFW(WriteASIC) {
	if (m483.reg <= 2) {
		MMC1_pwrap = SetPRG_slrom;
		MMC1_cwrap = SetCHR_slrom;
		MMC1_Write(A, V);
	} else {
		if (m483.reg <= 5) {
			Latch_Write(A, V);
		} else {
			MMC1_pwrap = SetPRG_serom;
			MMC1_cwrap = SetCHR_serom;
			MMC1_Write(A, V);
		}
	}
}

static void Reset(void) {
	m483.reg++;
	if (m483.reg >= 7) {
		m483.reg = 0;
	}
	Sync();
}

static void Power(void) {
	m483.reg = 0;
	Sync();
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper483_Init(CartInfo *info) {
	Latch_Init(info, SyncCNROM, NULL, 0, 0);
	MMC1_Init(info, MMC1B, 0, 0);

	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;

	AddExState(StateRegs, ~0, 0, NULL);
}
