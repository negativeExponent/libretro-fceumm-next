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

/* NES 2.0 Mapper 297 - 2-in-1 Uzi Lightgun (MGC-002) */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg[2];
} m297;

static SFORMAT StateRegs[] = {
	{ m297.reg, 2, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg16(A, 0x08 | (V & 0x07));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr4(A, 0x20 | (V & 0x1F));
}

static void Sync(void) {
	if (m297.reg[0] & 0x01) {
		/* MMC1 */
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncMirror();
	} else {
		/* Mapper 70 */
		setprg16(0x8000, ((m297.reg[0] & 0x02) << 1) | ((m297.reg[1] >> 4) & 0x03));
		setprg16(0xC000, ((m297.reg[0] & 0x02) << 1) | 0x03);
		setchr8(m297.reg[1] & 0x0F);
		setmirror(MI_V);
	}
}

static DECLFW(WriteMode) {
	if (A & 0x100) {
		m297.reg[0] = V;
		Sync();
	}
}

static DECLFW(WriteLatch) {
	if (m297.reg[0] & 0x01) {
		MMC1_Write(A, V);
	} else {
		m297.reg[1] = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m297, 0, sizeof(m297));
	MMC1_Power();
	SetWriteHandler(0x4100, 0x5FFF, WriteMode);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper297_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 0, 0);
	info->Power = Power;
	MMC1_cwrap = SetCHR;
	MMC1_pwrap = SetPRG;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
