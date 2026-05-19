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

/* NES 2.0 Mapper 404 - JY012005
 * 1998 Super HiK 8-in-1 (JY-021B)
 */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t reg;
} m404;

static SFORMAT StateRegs[] = {
	{ &m404.reg, 1, "EXPR" }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = (m404.reg & 0x40) ? 0x07 : 0x0F;
	setprg16(A, ((m404.reg << 3) & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr4(A, (m404.reg << 5) | (V & 0x1F));
}

static DECLFW(WriteReg) {
	if (!(m404.reg & 0x80)) {
		m404.reg = V;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m404, 0, sizeof(m404));
	MMC1_Reset();
}

static void Power(void) {
	memset(&m404, 0, sizeof(m404));
	MMC1_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper404_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	MMC1_cwrap = SetCHR;
	MMC1_pwrap = SetPRG;
	AddExState(StateRegs, ~0, 0, NULL);
}
