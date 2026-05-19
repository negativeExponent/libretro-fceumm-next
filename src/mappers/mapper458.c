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

/*
 * NES 2.0 Mapper 458 denotes the MMC3-based K-3102 (submapper 0) and GN-23
 * (submapper 1) multicart PCBs. The MMC3 is only used for CHR banking and
 * mirroring, while all PRG banking is controlled by an extra register, similar
 * to NES 2.0 Mapper 259.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m458;

static SFORMAT StateRegs[] = {
	{ &m458.reg, 1, "EXPR" },
	{ 0 }
};

static uint8_t dipsw;

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = 0x7F;
	uint16_t base = m458.reg << 4;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SyncPRG(void) {
	uint8_t prg = m458.reg & 0x0F;

	if (m458.reg & ((iNESCart.submapper == 1) ? 0x08 : 0x10)) {
		setprg32(0x8000, prg >> 1);
	} else {
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
	}
}

static DECLFR(ReadDIP) {
	if ((m458.reg & ((iNESCart.submapper == 1) ? 0x80 : 0x20)) && (dipsw & 0x03)) {
		return CartBR((A & ~0x1F) | (dipsw & 0x1F));
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m458.reg = A & 0xFF;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m458.reg = 0;
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m458.reg = 0;
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper458_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_SyncPRG = SyncPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
