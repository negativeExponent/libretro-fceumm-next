/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* Mapper 370 - F600
 * Golden Mario Party II - Around the World (6-in-1 multicart)
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 PPUCHRBus;
static uint8 mirr[8];

static void FP_FASTAPASS(1) M370PPU(uint32 A) {
	if ((mmc3.expregs[0] & 7) == 1) {
		A &= 0x1FFF;
		A >>= 10;
		PPUCHRBus = A;
		setmirror(MI_0 + mirr[A]);
	}
}

static void M370CW(uint32 A, uint8 V) {
	uint8 mask = (mmc3.expregs[0] & 4) ? 0xFF : 0x7F;
	mirr[A >> 10] = V >> 7;
	setchr1(A, (V & mask) | ((mmc3.expregs[0] & 7) << 7));
	if (((mmc3.expregs[0] & 7) == 1) && (PPUCHRBus == (A >> 10)))
		setmirror(MI_0 + (V >> 7));
}

static void M370PW(uint32 A, uint8 V) {
	uint8 mask = mmc3.expregs[0] & 0x20 ? 0x0F : 0x1F;
	setprg8(A, (V & mask) | ((mmc3.expregs[0] & 0x38) << 1));
}

static void M370MW(uint8 V) {
	mmc3.mirroring = V;
	if ((mmc3.expregs[0] & 7) != 1)
		setmirror((V & 1) ^ 1);
}

static DECLFR(M370Read) {
	return (mmc3.expregs[1] << 7);
}

static DECLFW(M370Write) {
	mmc3.expregs[0] = (A & 0xFF);
	FixMMC3PRG();
	FixMMC3CHR();
}

static void M370Reset(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] ^= 1;
	FCEU_printf("solderpad=%02x\n", mmc3.expregs[1]);
	MMC3RegReset();
}

static void M370Power(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 1; /* start off with the 6-in-1 menu */
	GenMMC3Power();
	SetReadHandler(0x5000, 0x5FFF, M370Read);
	SetWriteHandler(0x5000, 0x5FFF, M370Write);
}

void Mapper370_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.cwrap = M370CW;
	mmc3.pwrap = M370PW;
	mmc3.mwrap = M370MW;
	PPU_hook = M370PPU;
	info->Power = M370Power;
	info->Reset = M370Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
