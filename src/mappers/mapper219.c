/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

static uint8 reg[2];

static void M219PW(uint32 A, uint8 V) {
	setprg8(A, (reg[1] << 4) | (V & 0x0F));
}

static void M219CW(uint32 A, uint8 V) {
	setchr1(A, (reg[1] << 7) | (V & 0x7F));
}

static DECLFW(M219WriteOuter) {
	switch (A & 0x01) {
	case 0:
		reg[1] = (reg[1] & ~0x01) | ((V >> 3) & 0x01);
		MMC3_FixPRG();
		MMC3_FixCHR();
		break;
	case 1:
		reg[1] = (reg[1] & ~0x02) | ((V >> 4) & 0x02);
		MMC3_FixPRG();
		MMC3_FixCHR();
		break;
	}
}

static DECLFW(M219WriteASIC) {
	switch (A & 0xE001) {
	case 0x8000: /* Register index */
		MMC3_CMDWrite(A, V);
		if (A & 2) {
			reg[0] = V;
		}
		break;
	case 0x8001:
		if (!(reg[0] & 0x20)) { /* Scrambled mode inactive */
			MMC3_CMDWrite(A, V);
			break;
		}
		/* Scrambled mode active */
		if ((mmc3.cmd >= 0x08) && (mmc3.cmd <= 0x1F)) {
			/* Scrambled CHR register */
			uint8 index = (mmc3.cmd - 8) >> 2;
			if (mmc3.cmd & 0x01) {
				/* LSB nibble */
				mmc3.reg[index] &= ~0x0F;
				mmc3.reg[index] |= ((V >> 1) & 0x0F);
			} else {
				/* MSB nibble */
				mmc3.reg[index] &= ~0xF0;
				mmc3.reg[index] |= ((V << 4) & 0xF0);
			}
			MMC3_FixCHR();
		} else if ((mmc3.cmd >= 0x25) && (mmc3.cmd <= 0x26)) {
			/* Scrambled PRG register */
			V = ((V >> 5) & 0x01) | ((V >> 3) & 0x02) | ((V >> 1) & 0x04) | ((V << 1) & 0x08);
			mmc3.reg[6 | (mmc3.cmd & 0x01)] = V;
			MMC3_FixPRG();
		}
		break;
	default:
		MMC3_CMDWrite(A, V);
		break;
	}
}

static void M219Power(void) {
	reg[0] = 0;
	reg[1] = 3;
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, M219WriteOuter);
	SetWriteHandler(0x8000, 0x9FFF, M219WriteASIC);
}

static void M219Reset(void) {
	reg[0] = 0;
	reg[1] = 3;
	MMC3_Reset();
}

void Mapper219_Init(CartInfo *info) {
	MMC3_Init(info, 0, 0);
	MMC3_pwrap = M219PW;
	MMC3_cwrap = M219CW;
	info->Power = M219Power;
	info->Reset = M219Reset;
	AddExState(reg, 2, 0, "EXPR");
}
