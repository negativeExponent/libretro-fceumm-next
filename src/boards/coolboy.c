/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 CaH4e3, ClusteR
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
 * CoolBoy 400-in-1 FK23C-mimic mapper 16Mb/32Mb PROM + 128K/256K CHR RAM, optional SRAM, optional NTRAM
 * only MMC3 mode
 *
 * 6000 (xx76x210) | 0xC0
 * 6001 (xxx354x)
 * 6002 = 0
 * 6003 = 0
 *
 * hardware tested logic, don't try to understand lol
 */

#include "mapinc.h"
#include "mmc3.h"

static void COOLBOYCW(uint32 A, uint8 V) {
	uint32 mask = 0xFF ^ (mmc3.expregs[0] & 0x80);
	if (mmc3.expregs[3] & 0x10) {
		if (mmc3.expregs[3] & 0x40) { /* Weird mode */
			int cbase = (mmc3.cmd & 0x80) << 5;
			switch (cbase ^ A) { /* Don't even try to understand */
			case 0x0400:
			case 0x0C00: V &= 0x7F; break;
			}
		}
		/* Highest bit goes from MMC3 registers when mmc3.expregs[3]&0x80==0 or from mmc3.expregs[0]&0x08 otherwise */
		setchr1(A,
			(V & 0x80 & mask) | ((((mmc3.expregs[0] & 0x08) << 4) & ~mask)) /* 7th bit */
			| ((mmc3.expregs[2] & 0x0F) << 3) /* 6-3 bits */
			| ((A >> 10) & 7) /* 2-0 bits */
		);
	} else {
		if (mmc3.expregs[3] & 0x40) { /* Weird mode, again */
			int cbase = (mmc3.cmd & 0x80) << 5;
			switch (cbase ^ A) { /* Don't even try to understand */
			case 0x0000: V = mmc3.regs[0]; break;
			case 0x0800: V = mmc3.regs[1]; break;
			case 0x0400:
			case 0x0C00: V = 0; break;
			}
		}
		/* Simple MMC3 mode
		 * Highest bit goes from MMC3 registers when mmc3.expregs[3]&0x80==0 or from mmc3.expregs[0]&0x08 otherwise
		 */
		setchr1(A, (V & mask) | (((mmc3.expregs[0] & 0x08) << 4) & ~mask));
	}
}

static void COOLBOYPW(uint32 A, uint8 V) {
	uint32 mask = ((0x3F | (mmc3.expregs[1] & 0x40) | ((mmc3.expregs[1] & 0x20) << 2)) ^ ((mmc3.expregs[0] & 0x40) >> 2)) ^ ((mmc3.expregs[1] & 0x80) >> 2);
	uint32 base = ((mmc3.expregs[0] & 0x07) >> 0) | ((mmc3.expregs[1] & 0x10) >> 1) | ((mmc3.expregs[1] & 0x0C) << 2) | ((mmc3.expregs[0] & 0x30) << 2);

	/* Very weird mode
	 * Last banks are first in this mode, ignored when mmc3.cmd&0x40
	 */
	if ((mmc3.expregs[3] & 0x40) && (V >= 0xFE) && !((mmc3.cmd & 0x40) != 0)) {
		switch (A & 0xE000) {
		case 0xA000:
			if ((mmc3.cmd & 0x40)) V = 0;
			break;
		case 0xC000:
			if (!(mmc3.cmd & 0x40)) V = 0;
			break;
		case 0xE000:
			V = 0;
			break;
		}
	}

	/* Regular MMC3 mode, internal ROM size can be up to 2048kb! */
	if (!(mmc3.expregs[3] & 0x10))
		setprg8(A, (((base << 4) & ~mask)) | (V & mask));
	else { /* NROM mode */
		uint8 emask;
		mask &= 0xF0;
		if ((((mmc3.expregs[1] & 2) != 0))) /* 32kb mode */
			emask = (mmc3.expregs[3] & 0x0C) | ((A & 0x4000) >> 13);
		else /* 16kb mode */
			emask = mmc3.expregs[3] & 0x0E;
		setprg8(A, ((base << 4) & ~mask)   /* 7-4 bits are from base (see below) */
			| (V & mask)                   /* ... or from MM3 internal regs, depends on mask */
			| emask                        /* 3-1 (or 3-2 when (mmc3.expregs[3]&0x0C is set) from mmc3.expregs[3] */
			| ((A & 0x2000) >> 13));       /* 0th just as is */
	}
}

static DECLFW(COOLBOYWrite) {
	if(mmc3.wram & 0x80)
		CartBW(A,V);

	/* Deny any further writes when 7th bit is 1 AND 4th is 0 */
	if ((mmc3.expregs[3] & 0x90) != 0x80) {
		mmc3.expregs[A & 3] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void COOLBOYReset(void) {
	MMC3RegReset();
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
#if 0
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 0x60;
	mmc3.expregs[2] = 0;
	mmc3.expregs[3] = 0;
#endif
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void COOLBOYPower(void) {
	GenMMC3Power();
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
#if 0
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 0x60;
	mmc3.expregs[2] = 0;
	mmc3.expregs[3] = 0;
#endif
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
	SetWriteHandler(0x5000, 0x5fff, CartBW);            /* some games access random unmapped areas and crashes because of KT-008 PCB hack in MMC3 source lol */
	SetWriteHandler(0x6000, 0x7fff, COOLBOYWrite);
}

void COOLBOY_Init(CartInfo *info) {
	GenMMC3_Init(info, 512, 256, 8, 0);
	pwrap = COOLBOYPW;
	cwrap = COOLBOYCW;
	info->Power = COOLBOYPower;
	info->Reset = COOLBOYReset;
	AddExState(mmc3.expregs, 4, 0, "EXPR");
}

/*------------------ MINDKIDS ---------------------------*/
/* A COOLBOY variant that works identically but puts the outer bank registers
 * in the $5xxx range instead of the $6xxx range.
 * The UNIF board name is MINDKIDS (submapper 1).
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_268
 */

static void MINDKIDSPower(void) {
	GenMMC3Power();
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
	SetWriteHandler(0x5000, 0x5fff, COOLBOYWrite);
}

void MINDKIDS_Init(CartInfo *info) {
	GenMMC3_Init(info, 2048, 256, 8, info->battery);
	pwrap = COOLBOYPW;
	cwrap = COOLBOYCW;
	info->Power = MINDKIDSPower;
	info->Reset = COOLBOYReset;
	AddExState(mmc3.expregs, 4, 0, "EXPR");
}

void Mapper268_Init(CartInfo *info) {
	/* Technically, the distinction between COOLBOY ($6000-$7FFF) and MINDKIDS ($5000-$5FFF) is based on a solder pad setting. */
	/* In NES 2.0, the submapper field is used to distinguish between the two settings. */
	if (info->submapper == 1)
		MINDKIDS_Init(info);
	else
		COOLBOY_Init(info);
}
