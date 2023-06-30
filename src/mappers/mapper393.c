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
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg[2];

static uint8 *CHRRAM;
static uint32 CHRRAMSIZE;

static void M393CW(uint32 A, uint8 V) {
	if (reg[0] & 0x08) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, (V & 0xFF) | (reg[0] << 8));
	}
}

static void M393PW(uint32 A, uint8 V) {
	if (reg[0] & 0x20) {
		if (reg[0] & 0x10) {
			setprg16(0x8000, (reg[0] << 3) | (reg[1] & 0x07));
			setprg16(0xC000, (reg[0] << 3) | 0x07);
		} else {
			setprg32(0x8000, (reg[0] << 2) | ((mmc3.reg[6] >> 2) & 0x03));
		}
	} else {
		setprg8(A, (V & 0x0F) | (reg[0] << 4));
	}
}

static DECLFW(M393Write8) {
	reg[1] = V;
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_FixPRG();
			MMC3_FixCHR();
		default:
			MMC3_CMDWrite(A, V);
			MMC3_FixPRG();
		}
		break;
	default:
		MMC3_Write(A, V);
		MMC3_FixPRG();
		break;
	}
}

static DECLFW(M393Write6) {
	if (MMC3_WRAMWritable(A)) {
		reg[0] = A & 0xFF;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M393Power(void) {
	reg[0] = reg[1] = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M393Write6);
	SetWriteHandler(0x8000, 0xFFFF, M393Write8);
}

static void M393Reset(void) {
	reg[0] = reg[1] = 0;
	MMC3_Reset();
}

static void M393lose(void) {
	MMC3_Close();
	if (CHRRAM) {
		FCEU_free(CHRRAM);
	}
	CHRRAM = NULL;
}

void Mapper393_Init(CartInfo *info) {
	MMC3_Init(info, 8, 0);
	MMC3_pwrap = M393PW;
	MMC3_cwrap = M393CW;
	info->Power = M393Power;
	info->Reset = M393Reset;
	info->Close = M393lose;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(reg, 2, 0, "EXPR");
}
