/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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

static uint8 *CHRRAM;
static uint32 CHRRAMSIZE;

static void M393CW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 8)
		setchr8r(0x10, 0);
	else
		setchr1(A, (V & 0xFF) | (mmc3.expregs[0] << 8));
}

static void M393PW(uint32 A, uint8 V) {
	switch ((mmc3.expregs[0] >> 4) & 3) {
		case 0:
		case 1:
			setprg8(A, (V & 0x0F) | (mmc3.expregs[0] << 4));
			break;
		case 2:
			setprg32(0x8000, ((mmc3.regs[6] >> 2) & 3) | (mmc3.expregs[0] << 2));
			break;
		case 3:
			setprg16(0x8000, (mmc3.expregs[0] << 3) | (mmc3.expregs[1] & 7));
			setprg16(0xC000, (mmc3.expregs[0] << 3) | 7);
			break;
	}
}

static DECLFW(M393Write8) {
	switch (A & 0xE000) {
		case 0x8000:
		case 0xA000:
			MMC3_CMDWrite(A, V);
			break;
		case 0xC000:
		case 0xE000:
			MMC3_IRQWrite(A, V);
			break;
	}

	mmc3.expregs[1] = V;
	FixMMC3CHR();
	FixMMC3PRG();
}

static DECLFW(M393Write6) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = A & 0xFF;
		FixMMC3PRG();
		FixMMC3CHR();
	}
}

static void M393Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M393Write6);
	SetWriteHandler(0x8000, 0xFFFF, M393Write8);
}

static void M393Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

static void M393lose(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_free(CHRRAM);
	CHRRAM = NULL;
}

void Mapper393_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = M393PW;
	mmc3.cwrap = M393CW;
	info->Power = M393Power;
	info->Reset = M393Reset;
	info->Close = M393lose;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
