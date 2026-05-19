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
	uint8_t dipsw;
} m506;

static SFORMAT StateRegs[] = {
	{ &m506.reg, 1, "EXPR" },
	{ &m506.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (!(m506.reg & 0x03) ? 0x0F : 0x1F);
	uint16_t base = ((m506.reg << 5) & 0x20) | ((m506.reg << 3) & 0x10);

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = ((m506.reg & 0x03) ? 0x7F : 0xFF);
	uint16_t base = ((m506.reg << 7) & 0x100) | ((~m506.reg << 7) & 0x80);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m506.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	m506.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	m506.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper506_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
