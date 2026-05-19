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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
} m420;

static SFORMAT StateRegs[] = {
	{ m420.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m420.reg[0] & 0x80) {
		setprg32(0x8000, ((m420.reg[2] >> 2) & 0x08) | ((m420.reg[0] >> 1) & 0x07));
	} else {
		uint8_t mask = (m420.reg[0] & 0x20) ? 0x0F : ((m420.reg[3] & 0x20) ? 0x1F : 0x3F);
		uint8_t base = (m420.reg[3] << 3) & 0x20;

		setprg8(A, base | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m420.reg[1] & 0x80) ? 0x7F : 0xFF;
	uint16_t base = ((m420.reg[1] << 1) & 0x100) | ((m420.reg[1] << 5) & 0x80);

	setchr1(A, base | (V & mask));
}

static DECLFW(WriteReg) {
	/* writes possible regardless of MMC3 wram state */
	CartBW(A, V);
	m420.reg[A & 0x03] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m420, 0, sizeof(m420));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m420, 0, sizeof(m420));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper420_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
