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

#include "mapinc.h"
#include "mmc3.h"

static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSIZE = 0;

static void M165CW(uint32 A, uint8 V) {
	if (V == 0)
		setchr4r(0x10, A, 0);
	else
		setchr4(A, V >> 2);
}

static void M165PPUFD(void) {
	if (mmc3.expregs[0] == 0xFD) {
		M165CW(0x0000, mmc3.regs[0]);
		M165CW(0x1000, mmc3.regs[2]);
	}
}

static void M165PPUFE(void) {
	if (mmc3.expregs[0] == 0xFE) {
		M165CW(0x0000, mmc3.regs[1]);
		M165CW(0x1000, mmc3.regs[4]);
	}
}

static void M165CWM(uint32 A, uint8 V) {
	if (((mmc3.cmd & 0x7) == 0) || ((mmc3.cmd & 0x7) == 2))
		M165PPUFD();
	if (((mmc3.cmd & 0x7) == 1) || ((mmc3.cmd & 0x7) == 4))
		M165PPUFE();
}

static void FP_FASTAPASS(1) M165PPU(uint32 A) {
	if ((A & 0x1FF0) == 0x1FD0) {
		mmc3.expregs[0] = 0xFD;
		M165PPUFD();
	} else if ((A & 0x1FF0) == 0x1FE0) {
		mmc3.expregs[0] = 0xFE;
		M165PPUFE();
	}
}

static void M165Close(void) {
    if (CHRRAM) {
        FCEU_free(CHRRAM);
        CHRRAM = NULL;
    }
}

static void M165Power(void) {
	mmc3.expregs[0] = 0xFD;
	GenMMC3Power();
}

void Mapper165_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
    info->Close = M165Close;
	MMC3_cwrap = M165CWM;
	PPU_hook = M165PPU;
	info->Power = M165Power;
	CHRRAMSIZE = 4096;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(mmc3.expregs, 4, 0, "EXPR");
}