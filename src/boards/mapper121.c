/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007-2008 Mad Dumper, CaH4e3
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
 * Panda prince pirate.
 * MK4, MK6, A9711/A9713 board
 * 6035052 seems to be the same too, but with prot array in reverse
 * A9746  seems to be the same too, check
 * 187 seems to be the same too, check (A98402 board)
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static void Sync() {
	switch (mmc3.expregs[5] & 0x3F) {
		case 0x20:
			mmc3.expregs[7] = 1;
			mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x29:
			mmc3.expregs[7] = 1;
			mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x26:
			mmc3.expregs[7] = 0;
			mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x2B:
			mmc3.expregs[7] = 1;
			mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x2C:
			mmc3.expregs[7] = 1;
			if (mmc3.expregs[6])
				mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x3C:
		case 0x3F:
			mmc3.expregs[7] = 1;
			mmc3.expregs[0] = mmc3.expregs[6];
			break;
		case 0x28:
			mmc3.expregs[7] = 0;
			mmc3.expregs[1] = mmc3.expregs[6];
			break;
		case 0x2A:
			mmc3.expregs[7] = 0;
			mmc3.expregs[2] = mmc3.expregs[6];
			break;
		case 0x2F:
			break;
		default:
			mmc3.expregs[5] = 0;
			break;
	}
}

static void M121CW(uint32 A, uint8 V) {
	if (PRGsize[0] == CHRsize[0]) { /* A9713 multigame extension hack! */
		setchr1(A, V | ((mmc3.expregs[3] & 0x80) << 1));
	} else {
		if ((A & 0x1000) == ((mmc3.cmd & 0x80) << 5))
			setchr1(A, V | 0x100);
		else
			setchr1(A, V);
	}
}

static void M121PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x1F) | ((mmc3.expregs[3] & 0x80) >> 2));
	if (mmc3.expregs[5] & 0x3F) {
		setprg8(0xE000, (mmc3.expregs[0]) | ((mmc3.expregs[3] & 0x80) >> 2));
		setprg8(0xC000, (mmc3.expregs[1]) | ((mmc3.expregs[3] & 0x80) >> 2));
		setprg8(0xA000, (mmc3.expregs[2]) | ((mmc3.expregs[3] & 0x80) >> 2));
	}
}

static DECLFW(M121Write) {
	/*	FCEU_printf("write: %04x:%04x\n",A&0xE003,V); */
	switch (A & 0xE003) {
		case 0x8000:
/*		mmc3.expregs[5] = 0; */
/*		FCEU_printf("gen: %02x\n",V); */
			MMC3_CMDWrite(A, V);
			FixMMC3PRG(mmc3.cmd);
			break;
		case 0x8001:
			mmc3.expregs[6] = ((V & 1) << 5) | ((V & 2) << 3) | ((V & 4) << 1) |
				((V & 8) >> 1) | ((V & 0x10) >> 3) | ((V & 0x20) >> 5);
/*			FCEU_printf("bank: %02x (%02x)\n",V,mmc3.expregs[6]); */
			if (!mmc3.expregs[7])
				Sync();
			MMC3_CMDWrite(A, V);
			FixMMC3PRG(mmc3.cmd);
			break;
		case 0x8003:
			mmc3.expregs[5] = V;
			/*		mmc3.expregs[7] = 0; */
			/*		FCEU_printf("prot: %02x\n",mmc3.expregs[5]); */
			Sync();
			MMC3_CMDWrite(0x8000, V);
			FixMMC3PRG(mmc3.cmd);
			break;
	}
}

static uint8 prot_array[16] = { 0x83, 0x83, 0x42, 0x00 };
static DECLFW(M121LoWrite) {
	mmc3.expregs[4] = prot_array[V & 3]; /* 0x100 bit in address seems to be switch arrays 0, 2, 2, 3 (Contra Fighter) */
	if ((A & 0x5180) == 0x5180) { /* A9713 multigame extension */
		mmc3.expregs[3] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
	/*	FCEU_printf("write: %04x:%04x\n",A,V); */
}

static DECLFR(M121Read) {
	/*	FCEU_printf("read:  %04x->\n",A,mmc3.expregs[0]); */
	return mmc3.expregs[4];
}

static void M121Power(void) {
	mmc3.expregs[3] = 0x80;
	mmc3.expregs[5] = 0;
	GenMMC3Power();
	SetReadHandler(0x5000, 0x5FFF, M121Read);
	SetWriteHandler(0x5000, 0x5FFF, M121LoWrite);
	SetWriteHandler(0x8000, 0x9FFF, M121Write);
}

void Mapper121_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M121PW;
	mmc3.cwrap = M121CW;
	info->Power = M121Power;
	AddExState(mmc3.expregs, 8, 0, "EXPR");
}
