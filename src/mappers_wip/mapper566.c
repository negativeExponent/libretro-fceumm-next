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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m566;

static SFORMAT StateRegs[] = {
	{ &m566.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m566.reg & 0x04) ? 0x0F : 0x1F;
	uint16_t base = ((m566.reg << 2) & 0x20) | ((m566.reg << 4) & 0x10) | ((m566.reg << 1) & 0xC0);

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = m566.reg & 0x02 ? 0x7F : 0xFF;
	uint16_t base = m566.reg << 7 & 0x80 | m566.reg << 4 & 0x700;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SyncMirror(void) {
	if (m566.reg & 0x80) {
		uint8_t mirr = MMC3_GetCHRBank(0) >> 7;
		setmirror(MI_0 +  mirr);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	m566.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static void Reset() {
	m566.reg = 0;
	MMC3_Reset();
}

static void Power() {
	m566.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper566_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	MMC3_SyncMirror = SyncMirror;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
