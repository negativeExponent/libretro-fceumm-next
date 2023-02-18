/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023 - fixed mirroring
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

static uint8 *CHRRAM     = NULL;
static uint32 CHRRAMSIZE = 0;

extern uint8 *ExtraNTARAM;

static void M356CW(uint32 A, uint8 V) {
	if (EXPREGS[2] & 0x20) {
		uint32 NV = V & (0xFF >> (~EXPREGS[2] & 0xF));
		NV |= (EXPREGS[0] | ((EXPREGS[2] << 4) & 0xF00));
		setchr1(A, NV);
	} else
		setchr8r(0x10, 0);
}

static void M356PW(uint32 A, uint8 V) {
	uint8 MV = V & ((EXPREGS[3] & 0x3F) ^ 0x3F);
	MV |= (EXPREGS[1] | ((EXPREGS[2] << 2) & 0x300));
	setprg8(A, MV);
}

static void M356MW(uint8 V) {
	if (EXPREGS[2] & 0x40) {
		SetupCartMirroring(4, 1, ExtraNTARAM);
	} else {
		A000B = V;
		SetupCartMirroring((V & 1) ^ 1, 0, 0);
	}
}

static DECLFW(M356Write) {
	if (!(EXPREGS[3] & 0x40)) {
		EXPREGS[EXPREGS[4]] = V;
		EXPREGS[4] = (EXPREGS[4] + 1) & 3;
		FixMMC3PRG(MMC3_cmd);
		FixMMC3CHR(MMC3_cmd);
	}
}

static void M356Close(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_free(CHRRAM);
	CHRRAM = NULL;
}

static void M356Reset(void) {
	EXPREGS[0] = EXPREGS[1] = EXPREGS[3] = EXPREGS[4] = 0;
	EXPREGS[2] = 0x0F;
	MMC3RegReset();
}

static void M356Power(void) {
	EXPREGS[0] = EXPREGS[1] = EXPREGS[3] = EXPREGS[4] = 0;
	EXPREGS[2] = 0x0F;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M356Write);
}

void Mapper356_Init(CartInfo *info) {
	GenMMC3_Init(info, 512, 256, 0, 0);
	cwrap = M356CW;
	pwrap = M356PW;
	mwrap = M356MW;
	info->Reset = M356Reset;
	info->Power = M356Power;
	info->Close = M356Close;
	AddExState(EXPREGS, 5, 0, "EXPR");

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");

	if (!ExtraNTARAM) {
		ExtraNTARAM = (uint8 *)FCEU_gmalloc(2048);
		AddExState(ExtraNTARAM, 2048, 0, "EXNR");
	}
}
