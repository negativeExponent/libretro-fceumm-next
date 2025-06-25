/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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

/* iNES Mapper 205
 * UNIF boardname BMC-JC-016-2
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8 reg;
} m205;

static uint8 dipsw;

static SFORMAT StateRegs[] = {
	{ &m205.reg, 1, "REGS" },
	{ &dipsw, 1, "DPSW" },
	{ 0 }
};

static void SetPRG(uint16 A, uint16 V) {
	uint16 mask = (m205.reg & 0x02) ? 0x0F : 0x1F;
	uint16 base = m205.reg << 4;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16 A, uint16 V) {
	uint16 mask = (m205.reg & 0x02) ? 0x7F : 0xFF;
	uint16 base = m205.reg << 7;

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	m205.reg = V;
	if ((V & 0x01) && dipsw) {
		m205.reg |= 0x02;
	}
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m205, 0, sizeof(m205));
	dipsw = (dipsw + 1) & 0x01; /* solder pad */
	MMC3_Reset();
}

static void Power(void) {
	memset(&m205, 0, sizeof(m205));
	dipsw = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper205_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
