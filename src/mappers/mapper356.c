/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 356 -  J.Y. Company's 7-in-1 Rockman (JY-208)
 * All registers work as INES Mapper 045, except $6000 sequential register 2 (third write):
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m356;

static SFORMAT StateRegs[] = {
	{ m356.reg, 4, "EXPR" },
	{ &m356.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ~m356.reg[3] & 0x3F;
	uint16_t base = ((m356.reg[2] << 2) & 0x300) | m356.reg[1];

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m356.reg[2] & 0x20) {
		uint16_t mask = 0xFF >> (~m356.reg[2] & 0xF);
		uint16_t base = ((m356.reg[2] << 4) & 0xF00) | m356.reg[0];

		setchr1(A, (base & ~mask) | (V & mask));
	} else {
		setchr8r(0x10, 0);
	}
}

static void SyncMirror(void) {
	if (m356.reg[2] & 0x40) {
		setmirror(MI_4);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	if (!(m356.reg[3] & 0x40)) {
		m356.reg[m356.cmd] = V;
		m356.cmd = (m356.cmd + 1) & 3;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static void Reset(void) {
	memset(&m356, 0, sizeof(m356));
	m356.reg[2] = 0x0F;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m356, 0, sizeof(m356));
	m356.reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper356_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	MMC3_SyncMirror = SyncMirror;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
