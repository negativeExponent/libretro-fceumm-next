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
} m490;

static SFORMAT StateRegs[] = {
	{ &m490.reg, 1, "EXPR" },
	{ &m490.dipsw, 1, "DPSW" },
	{ 0 }
};

static void SyncPRG(void) {
	if (m490.reg & 0x20)
		setprg32(0x8000, m490.reg >> 1);
	else {
		setprg16(0x8000, m490.reg);
		setprg16(0xC000, m490.reg);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = (m490.reg << 4);

	setchr1(A, ((base & ~mask) | (V & mask)));
}

static DECLFR(ReadDIP) {
	if (m490.reg & 0x80) {
		A &= 0x0F;
		A |= (m490.dipsw & 0x0F);
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m490.reg = (A & 0xFF);
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static void Reset(void) {
	m490.reg = 0;
	m490.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m490.reg = 0;
	m490.dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0xFFFF, WriteReg);
}

void Mapper490_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_SyncPRG = SyncPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
