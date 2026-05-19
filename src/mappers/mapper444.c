/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* NC7000M PCB, with incorrect UNIF MAPR BS-110 due to a mix-up. Submapper bits 0 and 1. denote the setting of two
 * solder info->submapper that configure CHR banking. */
/* NC8000M PCB, indicated by submapper bit 2. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m444;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m444.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ((iNESCart.submapper & 0x04) && (m444.reg & 0x02)) ? 0x1F : 0x0F;
	uint16_t base = m444.reg << 4;

	if (m444.reg & 0x04) { /* NROM */
		V = (mmc3.reg[6] & ~((m444.reg & 0x08) ? 0x01 : 0x03)) | ((A >> 13) & ((m444.reg & 0x08) ? 0x01 : 0x03));
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (iNESCart.submapper & 0x01) ? 0xFF : 0x7F;
	uint16_t base = ((m444.reg << 7) & ((iNESCart.submapper & 0x01) ? 0x00 : 0x80)) | ((m444.reg << ((iNESCart.submapper & 0x02) ? 4 : 7)) & 0x100);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if ((m444.reg & 0x0C) == 0x08) {
		return dipsw;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m444.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m444, 0, sizeof(m444));
	dipsw++;
	dipsw &= 3;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m444, 0, sizeof(m444));
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper444_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
