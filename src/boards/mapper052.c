/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSIZE;

static void M052PW(uint32 A, uint8 V) {
	uint8 mask = (mmc3.expregs[0] & 8) ? 0x0F : 0x1F;
	uint8 base = (mmc3.expregs[0] << 4) & 0x70;

	setprg8(A, (base & ~mask) | (V & mask));
}

static void M052CW(uint32 A, uint8 V) {
	uint32 mask = (mmc3.expregs[0] & 0x40) ? 0x7F : 0xFF;
	uint32 bank = (iNESCart.submapper == 14) ? (((mmc3.expregs[0] << 3) & 0x080) | ((mmc3.expregs[0] << 7) & 0x300)) :
		(((mmc3.expregs[0] << 3) & 0x180) | ((mmc3.expregs[0] << 7) & 0x200));
	uint8 ram = CHRRAM && (
		((iNESCart.submapper == 13) && ((mmc3.expregs[0] & 0x03) == 0x03)) ||
		((iNESCart.submapper == 14) && (mmc3.expregs[0] & 0x20)));

	if (ram) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, (bank & ~mask) | (V & mask));
	}
}

static DECLFW(M052Write) {
	if (MMC3CanWriteToWRAM()) {
		if (!(mmc3.expregs[0] & 0x80)) {
			mmc3.expregs[0] = V;
			MMC3_FixPRG();
			MMC3_FixCHR();
		} else {
			CartBW(A, V);
		}
	}
}

static void M052Close(void) {
	GenMMC3Close();
	if (CHRRAM) {
		FCEU_free(CHRRAM);
		CHRRAM = NULL;
	}
}

static void M052Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M052Power(void) {
	M052Reset();
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M052Write);
}

void Mapper052_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M052CW;
	MMC3_pwrap = M052PW;
	info->Reset = M052Reset;
	info->Power = M052Power;
	info->Close = M052Close;
	AddExState(mmc3.expregs, 1, 0, "EXPR");

	if (info->CRC32 == 0xA874E216 && info->submapper != 13) {
		/* (YH-430) 97-98 Four-in-One */
		info->submapper = 13;
	} else if (info->CRC32 == 0xCCE8CA2F && info->submapper != 14) {
		/* Well 8-in-1 (AB128) (Unl) (p1), with 1024 PRG and CHR is incompatible with submapper 13.
		 * This was reassigned to submapper 14 instead. */
		info->submapper = 14;
	}

	if ((info->submapper == 13) || (info->submapper == 14)) {
		CHRRAMSIZE = info->CHRRamSize ? info->CHRRamSize : 8192;
		CHRRAM     = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
	}
}
