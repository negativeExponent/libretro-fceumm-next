/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

static void UNLA9746PWrap(uint32 A, uint8 V) {
	setprg8(A, (mmc3.expregs[1] << 4) | (V & 0x0F));
}

static void UNLA9746CWrap(uint32 A, uint8 V) {
	setchr1(A, (mmc3.expregs[1] << 7) | (V & 0x7F));
}

static DECLFW(UNLA9746WriteOuter) {
	switch (A & 1) {
		case 0:
			mmc3.expregs[1] = (mmc3.expregs[1] & ~1) | ((V >> 3) & 1);
			break;
		case 1:
			mmc3.expregs[1] = (mmc3.expregs[1] & ~2) | ((V >> 4) & 2);
			break;
	}
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static DECLFW(UNLA9746WriteASIC) {
	int index;

	if (A & 1) { /* Register data */
		if (~mmc3.expregs[0] & 0x20) { /* Scrambled mode inactive */
			MMC3_CMDWrite(A, V);
		} else { /* Scrambled mode active */
			if ((mmc3.cmd >= 0x08) && (mmc3.cmd <= 0x1F)) { /* Scrambled CHR register */
				index = (mmc3.cmd - 8) >> 2;
				if (mmc3.cmd & 1) { /* LSB nibble */
					mmc3.regs[index] &= ~0x0F;
					mmc3.regs[index] |= ((V >> 1) & 0x0F);
				} else { /* MSB nibble */
					mmc3.regs[index] &= ~0xF0;
					mmc3.regs[index] |= ((V << 4) & 0xF0);
				}
				FixMMC3CHR(mmc3.cmd);
			} else if ((mmc3.cmd >= 0x25) && (mmc3.cmd <= 0x26)) { /* Scrambled PRG register */
				mmc3.regs[6 | (mmc3.cmd & 1)] = ((V >> 5) & 1) | ((V >> 3) & 2) | ((V >> 1) & 4) | ((V << 1) & 8);
				FixMMC3PRG(mmc3.cmd);
			}
		}
	} else { /* Register index */
		MMC3_CMDWrite(A, V);
		if (A & 2)
			mmc3.expregs[0] = V;
	}
}

static void UNLA9746Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5FFF, UNLA9746WriteOuter);
	SetWriteHandler(0x8000, 0xBFFF, UNLA9746WriteASIC);
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 3;
	MMC3RegReset();
}

static void UNLA9746Reset(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 3;
	MMC3RegReset();
}

void UNLA9746_Init(CartInfo *info) {
	GenMMC3_Init(info, 128, 128, 0, 0);
	pwrap = UNLA9746PWrap;
	cwrap = UNLA9746CWrap;
	info->Power = UNLA9746Power;
	info->Reset = UNLA9746Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}
