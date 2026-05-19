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
} m570;

static SFORMAT StateRegs[] = {
	{ &m570.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m570.reg << 4;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m570.reg & 0x03) ? 0x0FF : 0x1FF;
	uint16_t base = !!(m570.reg & 0x03) * 0x200 | !!(m570.reg & 0x02) * 0x100;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m570.reg = A & 0xFF;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
}

static void Reset(void) {
	m570.reg = 0;
	VRC24_Reset();
}

static void Power(void) {
	m570.reg = 0;
	VRC24_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper570_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, FALSE, TRUE);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
