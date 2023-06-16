/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 CaH4e3
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
 * NES 2.0 Mapper 292 - PCB BMW8544
 * UNIF UNL-DRAGONFIGHTER
 * "Dragon Fighter" protected MMC3 based custom mapper board
 * mostly hacky implementation, I can't verify if this mapper can read a RAM of the
 * console or watches the bus writes somehow.
 * 
 * TODO: needs updating
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static void M292PW(uint32 A, uint8 V) {
	if (A == 0x8000)
		setprg8(A, mmc3.expregs[0] & 0x1F); /* the real hardware has this bank overrided with it's own register,
		                                * but MMC3 prg swap still works and you can actually change bank C000 at the
		                                * same time if use 0x46 cmd
		                                */
	else
		setprg8(A, V);
}

static void M292CW(uint32 A, uint8 V) {
	if (A == 0x0000)
		setchr2(0x0000, (V >> 1) ^ mmc3.expregs[1]);
	else if (A == 0x0800)
		setchr2(0x0800, (V >> 1) | ((mmc3.expregs[2] & 0x40) << 1));
	else if (A == 0x1000)
		setchr4(0x1000, mmc3.expregs[2] & 0x3F);
}

static DECLFW(M292ProtWrite) {
	if (!(A & 1)) {
		mmc3.expregs[0] = V;
		FixMMC3PRG();
	}
}

static DECLFR(M292ProtRead) {
	if (!fceuindbg) {
		if (!(A & 1)) {
			int tmp;
			if ((mmc3.expregs[0] & 0xE0) == 0xC0) {
				mmc3.expregs[1] = ARead[0x6a](0x6a);	/* program can latch some data from the BUS, but I can't say how exactly, */
			} else {							/* without more equipment and skills ;) probably here we can try to get any write */
				mmc3.expregs[2] = ARead[0xff](0xff);	/* before the read operation */
			}
			/* TODO: Verify that nothing breaks here */
			tmp = mmc3.cmd;
			mmc3.cmd &= 0x7F;
			FixMMC3CHR();
			mmc3.cmd = tmp;	/* there are more different behaviour of the board that's not used by game itself, so unimplemented here and */
		}					/* actually will break the current logic ;) */
	}
	return 0;
}

static void M292Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x6FFF, M292ProtWrite);
	SetReadHandler(0x6000, 0x6FFF, M292ProtRead);
}

void Mapper292_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M292PW;
	mmc3.cwrap = M292CW;
	info->Power = M292Power;
	AddExState(mmc3.expregs, 3, 0, "EXPR");
}
