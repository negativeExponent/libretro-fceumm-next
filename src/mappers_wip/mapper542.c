/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* NES 2.0 Mapper 542 denotes the unmarked PCB used in 毛泽东诞辰: 一百周年纪念
1893-1993 (Máozédōng Dànchén꞉ Yībǎi Zhōunián Jìniàn 1893-1993, Mao Zedong's
Birthday: 100th Anniversary 1893-1993). The game is a modification of 英雄傳 -
World Hero, which is assigned either to mapper 23.1 or to 27.

It uses the same VRC4 clone (A0/A1), increases the PRG-ROM size from the
original's 128 KiB to 256 KiB, keeps the 512 KiB of CHR-ROM, additionally maps
another fixed part of PRG-ROM to $6000-$7FFF, and most interestingly, can map
CIRAM into CHR address space.
*/

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m542;

static SFORMAT StateRegs[] = {
	{ &m542.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x1F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0x1FF);
	if (m542.reg & 0x01) {
		setchr1r(0x10, 0x0C00, 1);
	}
}

static DECLFW(WriteReg) {
	if (A & 0x800) {
		m542.reg = A >> 12;
		VRC24_SyncCHR();
	} else {
		VRC24_Write(A, V);
	}
}

static void Power(void) {
	m542.reg = 0;
	VRC24_Power();
	setprg8(0x6000, 0x0F);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0xD000, 0xEFFF, WriteReg);
}

void Mapper542_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, 0, TRUE);
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, 0);
	SetupCartCHRMapping(0x10, NTARAM, 0x2000, TRUE);
}
