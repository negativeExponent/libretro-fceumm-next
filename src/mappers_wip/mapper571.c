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
#include "vrc24.h"

static struct {
	uint8_t reg;
} m571;

static SFORMAT StateRegs[] = {
	{ &m571.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m571.reg & 0x10) ? 0x1F : 0x0F;
	uint16_t base = m571.reg << 1;

	if (m571.reg & 0x20) {
		setprg8(A, (base & ~mask) | (V & mask));
	} else {
		if (m571.reg & 0x06) {
			setprg32(0x8000, m571.reg >> 1);
		} else {
			setprg16(0x8000, m571.reg);
			setprg16(0xC000, m571.reg);
		}
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m571.reg & 0x10) ? 0xFF : 0x7F;
	uint16_t base = m571.reg << 4;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m571.reg & 0x01)) {
		m571.reg = A & 0xFF;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		if (m571.reg & 0x10) {
			vrc24.A0 = 0x08;
			vrc24.A1 = 0x04;
		} else {
			vrc24.A0 = 0x04;
			vrc24.A1 = 0x08;
		}
	}
}

static void Reset(void) {
	m571.reg = 0;
	VRC24_Reset();
}

static void Power(void) {
	m571.reg = 0;
	VRC24_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper571_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, FALSE, TRUE);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
