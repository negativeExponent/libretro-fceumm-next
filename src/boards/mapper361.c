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

/* Mapper 361 (YY841101C):
 *  JY-009
 *  JY-018
 *  JY-019
 *  OK-411
*/		

#include "mapinc.h"
#include "mmc3.h"

static void M361PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x0f) | mmc3.expregs[0]);
}

static void M361CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | (mmc3.expregs[0] << 3));
}

static void M361Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static DECLFW(M361Write) {
	mmc3.expregs[0] = V & 0xF0;
	FixMMC3PRG();
	FixMMC3CHR();
}

static void M361Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x7000, 0x7fff, M361Write);
}

void Mapper361_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M361PW;
	mmc3.cwrap = M361CW;
	info->Power = M361Power;
	info->Reset = M361Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
