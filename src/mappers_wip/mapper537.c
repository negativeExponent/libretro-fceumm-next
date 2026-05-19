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

/* NES 2.0 Mapper 537 - BMC-JY-103 */
/* The UNIF dump of "(JY-103) 3-in-1" (JY4M4 MAPR) has a strange bank order and will not run with this emulation. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m537;

static SFORMAT StateRegs[] = {
	{ m537.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((m537.reg[0] & 0x80) ? 0x0F : 0x1F);
	uint16_t base = (((m537.reg[0] >> 1) & 0x20) | ((m537.reg[1] << 4) & 0x10));

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF;
	uint16_t base = (m537.reg[0] << 2);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m537.reg[A & 0x01] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m537, 0, sizeof(m537));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m537, 0, sizeof(m537));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper537_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
