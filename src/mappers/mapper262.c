/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 * 	Copyright (C) 2023-2025-2026 negativeExponent
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

/* NES 2.0 Mapper 262 - UNL-SHERO */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m262;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m262.reg, 1, "REGS" },
	{ 0 }
};

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	if (m262.reg & 0x40) {
		setchr8r(0x10, 0);
	} else {
		uint16_t base = ((m262.reg << 5) << (A >> 11) & 0x100);
		setchr1(A, base | V);
	}
}

static DECLFR(ReadDIP) {
	if (A & 0x100) {
		return (dipsw);
	}
	return cpu.openbus;
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m262.reg = (V & 0xFC) | ((V << 1) & 0x02) | ((V >> 1) & 0x01);
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	MMC3_Reset();
	dipsw ^= 0xFF;
}

static void Power(void) {
	dipsw = 0x00;
	MMC3_Power();
	SetReadHandler(0x4100, 0x5FFF, ReadDIP);
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
}

static void Close(void) {
	MMC3_Close();
}

void Mapper262_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank_mmc3;
	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
