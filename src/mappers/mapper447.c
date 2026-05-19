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

/* NES 2.0 Mapper 447 - KL-06 (VRC4 clone)
 * 1993 New 860-in-1 Over-Valued Golden Version Games multicart
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m447;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m447.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m447.reg << 4;

	if (m447.reg & 0x04) {
		if (!(m447.reg & 0x02)) {
			V = A >> 13;
			base = base | (vrc24.prg[V & 0x01] & mask);
			mask = 0x03;
		} else {
			V = A >> 13;
			base = base | (vrc24.prg[V & 0x01] & mask);
			mask = 0x01;
		}
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (m447.reg << 7) | (V & 0x7F));
}

static DECLFR(ReadDIP) {
	if ((A & 0x8000) && (m447.reg & 0x08)) {
		return CartBR((A & ~0x03) | (dipsw & 0x03));
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if ((vrc24.cmd & 0x01) && !(m447.reg & 0x01)) {
		m447.reg = A & 0xFF;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
	}
}

static void Reset(void) {
	m447.reg = 0;
	dipsw++;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
}

static void Power(void) {
	m447.reg = 0;
	dipsw = 0;
	VRC24_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper447_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 1, 1);
	info->Reset = Reset;
	info->Power = Power;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);
}
