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

/* NES 2.0 Mapper 550 - 7-in-1 1993 Chess Series (JY-015) */

#include "mapinc.h"
#include "mmc1.h"

static struct {
	uint8_t latch;
	uint8_t reg;
} m550;

static SFORMAT StateRegs[] = {
	{ &m550.latch, 1, "LATC" },
	{ &m550.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if ((m550.reg & 0x06) == 0x06) {
		setprg16(A, (m550.reg << 2) | (V & 0x07));
	} else {
		setprg32(0x8000, (m550.reg << 1) | ((m550.latch >> 4) & 0x01));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if ((m550.reg & 0x06) == 0x06) {
		setchr4(A, ((m550.reg << 2) & 0x18) | (V & 0x07));
	} else {
		setchr8(((m550.reg << 1) & 0x0C) | (m550.latch & 0x03));
	}
}

static DECLFW(WriteReg) {
	if (!(m550.reg & 0x08)) {
		m550.reg = A & 0x0F;
		MMC1_SyncPRG();
		MMC1_SyncCHR();
	}
	CartBW(A, V);
}

static DECLFW(WriteLatch) {
	m550.latch = V;
	if ((m550.reg & 0x06) == 0x06) {
		MMC1_Write(A, V);
	}
	MMC1_SyncPRG();
	MMC1_SyncCHR();
}

static void Reset(void) {
	memset(&m550, 0, sizeof(m550));
	MMC1_Reset();
}

static void Power(void) {
	memset(&m550, 0, sizeof(m550));
	MMC1_Power();
	SetWriteHandler(0x7000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper550_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 8, 0);
	info->Power = Power;
	info->Reset = Reset;
	MMC1_cwrap = SetCHR;
	MMC1_pwrap = SetPRG;
	AddExState(StateRegs, ~0, 0, NULL);
}
