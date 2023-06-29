/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* added 2020-1-28 - */
/* NES 2.0 Mapper 372 is used for a revision of the Rockman I-VI multicart (PCB ID SFC-12).
 * It is INES Mapper 045 but with one bit of outer bank register #2 working as a CHR-ROM/RAM switch.
 */

#include "mapinc.h"
#include "mmc3.h"

static uint32 CHRRAMSIZE;
static uint8 *CHRRAM;

static void M372CW(uint32 A, uint8 V) {
	if (mmc3.expregs[2] & 0x20) {
		setchr8r(0x10, 0);
	} else {
		uint32 mask = 0xFF >> (~mmc3.expregs[2] & 0xF);
		uint32 base = ((mmc3.expregs[2] << 4) & 0xF00) | mmc3.expregs[0];
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static void M372PW(uint32 A, uint8 V) {
	uint32 mask = ~mmc3.expregs[3] & 0x3F;
	uint32 base = ((mmc3.expregs[2] << 2) & 0x300) | mmc3.expregs[1];
	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(M372Write) {
	if (!(mmc3.expregs[3] & 0x40)) {
		mmc3.expregs[mmc3.expregs[4]] = V;
		mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M372Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	MMC3RegReset();
}

static void M372Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M372Write);
}

static void M372Close(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;
}

void Mapper372_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_cwrap = M372CW;
	MMC3_pwrap = M372PW;
	info->Reset = M372Reset;
	info->Power = M372Power;
	info->Close = M372Close;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(mmc3.expregs, 5, 0, "EXPR");
}
