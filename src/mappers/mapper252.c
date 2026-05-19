/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t chrMask;
	uint8_t chrCompare;
} m252;

static writefunc writePPU2007;
extern uint32_t RefreshAddr;

static SFORMAT StateRegs[] = {
	{ &m252.chrMask, 1, "CMSK" },
	{ &m252.chrCompare, 1, "CCMP" },
	{ 0 }
};

static void SetPRGBank_vrc24(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHRBank_vrc24(uint16_t A, uint16_t V) {
	if ((V & m252.chrMask) == m252.chrCompare) {
		setchr1r(0x10, A, V);
	} else {
		setchr1(A, V);
	}
}

static DECLFW(WritePPU2007) {
	if (RefreshAddr < 0x2000) {
		switch (vrc24.chr[RefreshAddr >> 10]) {
		case 0x88:
			m252.chrMask = 0xFC;
			m252.chrCompare = 0x4C;
			break;
		case 0xC2:
			m252.chrMask = 0xFE;
			m252.chrCompare = 0x7C;
			break;
		case 0xC8:
			m252.chrMask = 0xFE;
			m252.chrCompare = 0x04;
			break;
		}
	}
	writePPU2007(A, V);
}

static void Close(void) {
	VRC24_Close();
}

static void Power(void) {
	if (iNESCart.mapper == 252) {
		m252.chrMask = 0xFE;
		m252.chrCompare = 0x06;
	} else {
		m252.chrMask = 0xFE;
		m252.chrCompare = 0x04;
	}
	VRC24_Power();

	writePPU2007 = GetWriteHandler(0x2007);
	SetWriteHandler(0x2007, 0x2007, WritePPU2007);
}

void Mapper252_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, FALSE, TRUE);
	VRC24_pwrap = SetPRGBank_vrc24;
	VRC24_cwrap = SetCHRBank_vrc24;

	info->Power = Power;
	info->Close = Close;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = info->iNES2 ? (info->CHRRamSize + info->CHRRamSaveSize) : 2048;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
}

void Mapper253_Init(CartInfo *info) {
	Mapper252_Init(info);
}
