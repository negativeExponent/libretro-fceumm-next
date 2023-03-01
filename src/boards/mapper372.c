/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* added 2020-1-28 - */
/* NES 2.0 Mapper 372 is used for a revision of the Rockman I-VI multicart (PCB ID SFC-12).
 * It is INES Mapper 045 but with one bit of outer bank register #2 working as a CHR-ROM/RAM switch.
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 WRAM[0x2000];
static uint32 CHRRAMSIZE;
static uint8 *CHRRAM;

static void M372CW(uint32 A, uint8 V) {
	if (!UNIFchrrama) {
		uint32 NV = V;
		if (mmc3.expregs[2] & 8)
			NV &= (1 << ((mmc3.expregs[2] & 7) + 1)) - 1;
		else if (mmc3.expregs[2])
			NV &= 0; /* hack ;( don't know exactly how it should be */
		NV |= mmc3.expregs[0] | ((mmc3.expregs[2] & 0xF0) << 4);
		if (mmc3.expregs[2] & 0x20)
			setchr1r(0x10, A, V);
		else
			setchr1(A, NV);
	} else
		/* FIXME */
		/* setchr8(0); */ /* i don't know what cart need this, but a new one need other lol */
		setchr1(A, V);
}

static void M372PW(uint32 A, uint8 V) {
	uint32 MV = V & ((mmc3.expregs[3] & 0x3F) ^ 0x3F);
	MV |= mmc3.expregs[1];
	if (UNIFchrrama)
		MV |= ((mmc3.expregs[2] & 0x40) << 2);
	setprg8(A, MV);
	/*	FCEU_printf("1:%02x 2:%02x 3:%02x A=%04x V=%03x\n",mmc3.expregs[1],mmc3.expregs[2],mmc3.expregs[3],A,MV); */
}

static DECLFW(M372Write) {
	if (mmc3.expregs[3] & 0x40) {
		WRAM[A - 0x6000] = V;
		return;
	}
	mmc3.expregs[mmc3.expregs[4]] = V;
	mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static DECLFR(M372Read) {
	uint32 addr = 1 << (mmc3.expregs[5] + 4);
	if (A & (addr | (addr - 1)))
		return X.DB | 1;
	else
		return X.DB;
}

static void M372Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[5]++;
	mmc3.expregs[5] &= 7;
	MMC3RegReset();
}

static void M372Power(void) {
	GenMMC3Power();
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = mmc3.expregs[4] = mmc3.expregs[5] = 0;
	SetWriteHandler(0x5000, 0x7FFF, M372Write);
	SetReadHandler(0x5000, 0x5FFF, M372Read);
}

static void M372Close(void) {
	GenMMC3Close();
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;
}

void Mapper372_Init(CartInfo *info) {
	GenMMC3_Init(info, 512, 256, 8, info->battery);
	cwrap = M372CW;
	pwrap = M372PW;
	info->Reset = M372Reset;
	info->Power = M372Power;
	info->Close = M372Close;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(mmc3.expregs, 5, 0, "EXPR");
}
