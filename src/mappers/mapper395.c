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

/* Mapper 395 - Realtec 8210
 * Super Card 12-in-1 (SPC002)
 * Super Card 13-in-1 (SPC003)
 * Super Card 14-in-1 (King006)
 * Super Card 14-in-1 (King007)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m395;

static SFORMAT StateRegs[] = {
	{ m395.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m395.reg[1] & 0x08) ? 0x0F : 0x1F;
	uint16_t base = ((m395.reg[0] << 4) & 0x80) | ((m395.reg[0] << 1) & 0x60) | ((m395.reg[1] << 4) & 0x10);

	setprg8(A, base | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m395.reg[1] & 0x40) ? 0x7F : 0xFF;
	uint16_t base = ((m395.reg[0] << 4) & 0x300) | ((m395.reg[1] << 5) & 0x400) | ((m395.reg[1] << 3) & 0x80);

	setchr1(A, base | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m395.reg[1] & 0x80)) {
		m395.reg[(A >> 4) & 0x01] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m395, 0, sizeof(m395));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m395, 0, sizeof(m395));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper395_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
