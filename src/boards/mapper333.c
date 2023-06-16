/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2016 CaH4e3
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

#include "mapinc.h"
#include "mmc3.h"

static void M333CW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] & 0xC) << 5) | (V & 0x7F));
}

static void M333PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x10) { /* MMC3 mode */
		setprg8(A, ((mmc3.expregs[0] & 0xC) << 2) | (V & 0xF));
	} else {
		setprg32(0x8000, mmc3.expregs[0] & 0xF);
	}
}

static DECLFW(M333Write) {
	if (A & 0x1000) {
		mmc3.expregs[0] = V;
		FixMMC3PRG();
		FixMMC3CHR();
	} else {
		if (A < 0xC000)
			MMC3_CMDWrite(A, V);
		else
			MMC3_IRQWrite(A, V);
	}
}

static void M333Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x8000, 0xFFFF, M333Write);
}

void Mapper333_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = M333CW;
	mmc3.pwrap = M333PW;
	info->Power = M333Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}
