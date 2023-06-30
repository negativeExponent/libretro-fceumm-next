/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2017 CaH4e3
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

/* added on 2019-5-23 - NES 2.0 Mapper 260
 * HP10xx/HP20xx - a simplified version of FK23C mapper with pretty strict and better
 * organized banking behaviour. It seems that some 176 mapper assigned game may be
 * actually this kind of board instead but in common they aren't compatible at all,
 * the games on the regular FK23C boards couldn't run on this mapper and vice versa...
 */

static uint8 reg[4];
static uint8 cnrom_chr;
static uint8 dipsw;

static void M260CW(uint32 A, uint8 V) {
	uint16 base = reg[2] & 0x7F;

	if (reg[0] & 0x04) {
		switch (reg[0] & 0x03) {
		case 0: setchr8(base); break; /* NROM-128: 8 KiB CHR */
		case 1: setchr8(base); break; /* NROM-256: 8 KiB CHR */
		case 2: setchr8((base & ~0x01) | (cnrom_chr & 0x01)); break; /* CNROM: 16 KiB CHR */
		case 3: setchr8((base & ~0x03) | (cnrom_chr & 0x03)); break; /* CNROM: 32 KiB CHR */
		}
	} else {
		uint8 mask = (reg[0] & 0x01) ? 0x7F : 0xFF;

		setchr1(A, ((base << 3) & ~mask) | (V & mask));
	}
}

static void M260PW(uint32 A, uint8 V) {
	uint8 base = reg[1] & 0x3F;

	if (reg[0] & 0x04) {
		if (!(reg[0] & 0x03)) {
			setprg16(0x8000,  base);
			setprg16(0xC000,  base);
		} else {
			setprg32(0x8000, base >> 1);
		}
	} else {
		uint8 mask = (reg[0] & 0x02) ? 0x0F : 0x1F;

		setprg8(A, ((base << 1) & ~mask) | (V & mask));
	}
}

static DECLFW(M260HiWrite) {
	if((reg[0] & 0x07) >= 0x06 ) {
		cnrom_chr = V;
		MMC3_FixCHR();
	}
	MMC3_Write(A, V);
}

static DECLFW(M260Write) {
	if (!(reg[0] & 0x80)) {
		reg[A & 3] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static DECLFR(M260Read) {
	return dipsw;
}

static void M260Reset(void) {
	dipsw++;
	dipsw &= 0xF;
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	MMC3_Reset();
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static void M260Power(void) {
	dipsw = 0;
	reg[0] = reg[1] = reg[2] = reg[3] = 0;
	MMC3_Power();
	SetReadHandler(0x5000, 0x5fff, M260Read);
	SetWriteHandler(0x5000, 0x5fff, M260Write);
	SetWriteHandler(0x8000, 0xffff, M260HiWrite);
}

void Mapper260_Init(CartInfo *info) {
	MMC3_Init(info, 0, 0);
	MMC3_cwrap = M260CW;
	MMC3_pwrap = M260PW;
	info->Power = M260Power;
	info->Reset = M260Reset;
	AddExState(reg, 4, 0, "EXPR");
	AddExState(&cnrom_chr, 1, 0, "CHRC");
	AddExState(&dipsw, 1, 0, "DPSW");
}
