/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* Submapper 13/14 - CHR-ROM + CHR-RAM */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m052;

static SFORMAT StateRegs[] = {
	{ &m052, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = (m052.reg & 0x08) ? 0x0F : 0x1F;
	uint8_t base = (m052.reg << 4) & 0x70;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m052.reg & 0x40) ? 0x7F : 0xFF;
	uint16_t base = (((m052.reg << 3) & 0x180) | ((m052.reg << 7) & 0x200));
	uint8_t chrram = CHRRAMSIZE &&
	               (((iNESCart.submapper == 13) && ((m052.reg & 0x03) == 0x03)) ||
                   ((iNESCart.submapper == 14) && (m052.reg & 0x20)));

	if (iNESCart.CRC32 == 0x68FE207F) {
		/* Mario 7-in-1 (YH-705) with wrong bank order */
		base = ((m052.reg & 0x20) << 4) | ((m052.reg & 0x04) << 6) |
		       (m052.reg & 0x40 ? (m052.reg & 0x10) << 3 : 0x00);
	} else if (iNESCart.submapper == 14) {
		/* Well 8-in-1 (AB128) (Unl) (p1) */
		base = ((m052.reg << 3) & 0x080) | ((m052.reg << 7) & 0x300);
	}

	if (chrram) {
		setchr1r(0x10, A, (base & ~mask) | (V & mask));
	} else {
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		if (!(m052.reg & 0x80)) {
			m052.reg = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		} else {
			CartBW(A, V);
		}
	}
}

static void Reset(void) {
	memset(&m052, 0, sizeof(m052));
	MMC3_Reset();
}

static void Power(void) {
	memset(&m052, 0, sizeof(m052));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper052_Init(CartInfo *info) {
	uint8_t ws = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) / 1024 : 8;

	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;

	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	if (info->CRC32 == 0xA874E216 && info->submapper != 13) {
		info->submapper = 13; /* (YH-430) 97-98 Four-in-One */
		iNESCart.CHRRamSize = 8192;
	} else if (info->CRC32 == 0xCCE8CA2F && info->submapper != 14) {
		/* Well 8-in-1 (AB128) (Unl) (p1), with 1024 PRG and CHR is incompatible with submapper 13.
		 * This is reassigned to submapper 14 instead. */
		info->submapper = 14;
		iNESCart.CHRRamSize = 8192;
	}

	if (ROM.chr.size && iNESCart.CHRRamSize) {
		CHRRAMSIZE = info->CHRRamSize ? info->CHRRamSize : 8192;
		CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
	}
}
