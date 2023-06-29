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

#include "mapinc.h"
#include "mmc3.h"

static void M115PRG(void) {
	if (mmc3.expregs[0] & 0x80) {
		uint8 bank = mmc3.expregs[0] & 0x0F;
		if (mmc3.expregs[0] & 0x20) {
			setprg32(0x8000, bank >> 1); /* real hardware tests, info 100% now lol */
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(0x8000, mmc3.regs[6]);
		setprg8(0xA000, mmc3.regs[7]);
		setprg8(0xC000, ~1);
		setprg8(0xE000, ~0);
	}
}

static void M115CW(uint32 A, uint8 V) {
	uint16 base = (mmc3.expregs[1] & 1) << 8;
	setchr1(A, base | V);
}

static DECLFW(M115WriteLow) {
	mmc3.expregs[A & 0x01] = V;
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static DECLFW(M115Write) {
	A &= 0xE001;
	switch (A) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 6:
		case 7:
			mmc3.regs[mmc3.cmd & 0x07] = V;
			MMC3_FixPRG();
			break;
		default:
			MMC3_CMDWrite(A, V);
			break;
		}
	default:
		MMC3_CMDWrite(A, V);
		break;
	}
}

static void M115Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M115WriteLow);
	SetWriteHandler(0x8000, 0x9FFF, M115Write);
}

void Mapper115_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_cwrap = M115CW;
	MMC3_FixPRG = M115PRG;
	info->Power = M115Power;
	AddExState(mmc3.expregs, 3, 0, "EXPR");
}
