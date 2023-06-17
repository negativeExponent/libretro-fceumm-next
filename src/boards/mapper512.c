/* FCE Ultra - NES/Famicom Emulator
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
 *
 * 8-in-1  Rockin' Kats, Snake, (PCB marked as "8 in 1"), similar to 12IN1,
 * but with MMC3 on board, all games are hacked the same, Snake is buggy too!
 *
 * no reset-citcuit, so selected game can be reset, but to change it you must use power
 *
 */

/* NES 2.0 Mapper 512 is used for 中國大亨 (Zhōngguó Dàhēng) */

#include "mapinc.h"
#include "mmc3.h"

static uint8 *CHRRAM     = NULL;
static uint32 CHRRAMSIZE = 8192;

extern uint8 *ExtraNTARAM;

static void M512MW(uint8 V) {
	if (mmc3.expregs[0] == 1) {
		SetupCartMirroring(4, 1, ExtraNTARAM);
	} else {
		mmc3.mirroring = V;
		SetupCartMirroring((V & 1) ^ 1, 0, 0);
	}
}

static void M512CW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 2)
		setchr1r(0x10, A, (V & 0x03));
	else
		setchr1(A, V);
}

static void M512PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x3F));
}

static DECLFW(M512Write) {
	if (A & 0x100) {
		mmc3.expregs[0] = V & 3;
		MMC3_FixCHR();
	}
}

static void M512Close(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;
}

static void M512Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x4100, 0x4FFF, M512Write);
}

void Mapper512_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M512CW;
	MMC3_pwrap = M512PW;
	MMC3_mwrap = M512MW;
	info->Power = M512Power;
	info->Close = M512Close;
	AddExState(mmc3.expregs, 1, 0, "EXPR");

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8*)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");

	if (!ExtraNTARAM) {
		ExtraNTARAM = (uint8 *)FCEU_gmalloc(2048);
		AddExState(ExtraNTARAM, 2048, 0, "EXNR");
	}
}
