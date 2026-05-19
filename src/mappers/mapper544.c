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
 */

/* NES 2.0 Mapper 544 - Waixing FS306 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t nt[4];
	uint8_t prg;
	uint8_t chrMask;
	uint8_t chrCompare;
} m544;

static writefunc writePPU;
extern uint32_t RefreshAddr;

static SFORMAT StateRegs[] = {
	{ m544.nt, 4, "NTBL" },
	{ &m544.prg, 1, "PRGC" },
	{ &m544.chrMask, 1, "CMSK" },
	{ &m544.chrCompare, 1, "CCMP" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (A == 0xC000) {
		V = m544.prg;
	}
	setprg8(A, V & 0x1F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	if ((V & m544.chrMask) == m544.chrCompare) {
		setchr1r(0x10, A, V);
	} else {
		setchr1(A, V);
	}
}

static DECLFW(WriteMisc) {
	if (A & 0x04) {
		m544.nt[A & 0x03] = V & 0x01;
		setmirrorw(m544.nt[0], m544.nt[1], m544.nt[2], m544.nt[3]);
	} else {
		m544.prg = V;
		VRC24_SyncPRG();
	}
}

static const uint8_t compareMasks[8] = {
    0x28, 0x00, 0x4C, 0x64, 0x46, 0x7C, 0x04, 0xFF
};

static DECLFW(WritePPU2007) {
	if (RefreshAddr < 0x2000) {
		uint8_t reg = RefreshAddr >> 10;
		uint8_t chrBank = vrc24.chr[reg];
		if (chrBank & 0x80) {
			if (chrBank & 0x10) {
				m544.chrMask = 0x00;
				m544.chrCompare = 0xFF;
			} else {
				m544.chrMask = (chrBank & 0x40) ? 0xFE : 0xFC;
				m544.chrCompare = compareMasks[((chrBank >> 1) & 0x01) | ((chrBank >> 2) & 0x02) | ((chrBank >> 4) & 0x04)];
			}
			VRC24_SyncCHR();
		}
	}
	writePPU(A, V);
}

static void Power(void) {
	memset(&m544, 0, sizeof(m544));
	m544.chrMask = 0xFC;
	m544.chrCompare = 0x28;
	m544.nt[0] = 0;
	m544.nt[1] = 0;
	m544.nt[2] = 1;
	m544.nt[3] = 1;
	m544.prg = ~1;
	VRC24_Power();
	writePPU = GetWriteHandler(0x2007);
	SetWriteHandler(0x2007, 0x2007, WritePPU2007);
}

void Mapper544_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x400, 0x800, 1, 1);
	info->Power = Power;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	VRC24_WriteExtSelect = WriteMisc;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAMSIZE = 2048;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}
