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

static struct {
	uint8_t reg;
} m512;

static SFORMAT StateRegs[] = {
	{ &m512.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, (V & 0x3F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (m512.reg & 0x02) {
		setchr1r(0x10, A, (V & 0x03));
	} else {
		setchr1(A, V & 0xFF);
	}
}

static void SyncMirror(void) {
	if (m512.reg == 1) {
		SetupCartMirroring(4, 0, &CHRRAM[4096]);
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m512.reg = V & 0x03;
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static void Power(void) {
	m512.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x4100, 0x4FFF, WriteReg);
}

void Mapper512_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	MMC3_SyncMirror = SyncMirror;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
}
