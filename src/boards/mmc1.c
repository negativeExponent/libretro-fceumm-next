/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
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

#include "mmc1.h"

static uint32 WRAMSIZE = 0;

 /* size of non-battery-backed portion of WRAM */
 /* serves as starting offset for actual save ram from total wram size */
 /* returns 0 if entire work ram is battery backed ram */
static uint32 NONSaveRAMSIZE = 0;

static uint8 *WRAM = NULL;
static MMC1Type mmc1_type = MMC1B;

MMC1 mmc1;

static void GENWRAMWRAP(void) {
	uint8 bank = 0;

	if (!WRAMSIZE) {
		return;
	}
	if (WRAMSIZE > 8192) {
		if (WRAMSIZE > 16384) {
			bank = (mmc1.regs[1] >> 2) & 3;
		} else {
			bank = (mmc1.regs[1] >> 3) & 1;
		}
	}
	setprg8r(0x10, 0x6000, bank);
}

static void GENPWRAP(uint32 A, uint8 V) {
	setprg16(A, V & 0x0F);
}

static void GENCWRAP(uint32 A, uint8 V) {
	setchr4(A, V & 0x1F);
}

static uint8 MMC1WRAMEnabled(void) {
	if ((mmc1.regs[3] & 0x10) && (mmc1_type == MMC1B)) {
		return FALSE;
	}

	return TRUE;
}

static DECLFW(MBWRAM) {
	if (MMC1WRAMEnabled()) {
		/* WRAM is enabled. */
		CartBW(A, V);
	}
}

static DECLFR(MAWRAM) {
	if (!MMC1WRAMEnabled()) {
		/* WRAM is disabled */
		return X.DB;
	}

	return CartBR(A);
}

uint32 MMC1GetPRGBank(int index) {
	uint32 bank;
	uint8 prg = mmc1.regs[3];

	switch (mmc1.regs[0] & 0xC) {
	case 0xC:
		bank = prg | (index * 0x0F);
		break;
	case 0x8:
		bank = (prg & (index * 0x0F));
		break;
	case 0x0:
	case 0x4:
	default:
		bank = (prg & ~1) | index;
		break;
	}

	if ((mmc1.regs[3] & 0x10) && (mmc1_type == MMC1A)) {
		return ((bank & 0x07) | (mmc1.regs[3] & 0x08));
	}

	return (bank & 0x0F);
}

uint32 MMC1GetCHRBank(int index) {
	if (mmc1.regs[0] & 0x10) {
		return (mmc1.regs[1 + index]);
	}

	return ((mmc1.regs[1] & ~1) | index);
}

void FixMMC1CHR(void) {
	if (mmc1.wwrap) {
		mmc1.wwrap();
	}

	mmc1.cwrap(0x0000, MMC1GetCHRBank(0));
	mmc1.cwrap(0x1000, MMC1GetCHRBank(1));
}

void FixMMC1PRG(void) {
	mmc1.pwrap(0x8000, MMC1GetPRGBank(0));
	mmc1.pwrap(0xc000, MMC1GetPRGBank(1));
}

void FixMMC1MIRRORING(void) {
	switch (mmc1.regs[0] & 3) {
	case 2: setmirror(MI_V); break;
	case 3: setmirror(MI_H); break;
	case 0: setmirror(MI_0); break;
	case 1: setmirror(MI_1); break;
	}
}

static uint64 lreset;
DECLFW(MMC1Write) {
	int n = (A >> 13) - 4;

	/* The MMC1 is busy so ignore the write. */
	/* As of version FCE Ultra 0.81, the timestamp is only
		increased before each instruction is executed(in other words
		precision isn't that great), but this should still work to
		deal with 2 writes in a row from a single RMW instruction.
	*/
	if ((timestampbase + timestamp) < (lreset + 2)) {
		return;
	}

	/* FCEU_printf("Write %04x:%02x\n",A,V); */
	if (V & 0x80) {
		mmc1.regs[0] |= 0xC;
		mmc1.shift = mmc1.buffer = 0;
		FixMMC1PRG();
		lreset = timestampbase + timestamp;
		return;
	}

	mmc1.buffer |= (V & 1) << (mmc1.shift++);

	if (mmc1.shift == 5) {
		/* FCEU_printf("REG[%d]=%02x\n",n,mmc1.buffer); */
		mmc1.regs[n] = mmc1.buffer;
		mmc1.shift = mmc1.buffer = 0;
		switch (n) {
		case 0:
			FixMMC1MIRRORING();
			FixMMC1CHR();
			FixMMC1PRG();
			break;
		case 1:
			FixMMC1CHR();
			FixMMC1PRG();
			break;
		case 2:
			FixMMC1CHR();
			break;
		case 3:
			FixMMC1PRG();
			break;
		}
	}
}

void GenMMC1Restore(int version) {
	FixMMC1MIRRORING();
	FixMMC1CHR();
	FixMMC1PRG();
	lreset = 0; /* timestamp(base) is not stored in save states. */
}

void MMC1RegReset(void) {
	int i;

	for (i = 0; i < 4; i++) {
		mmc1.regs[i] = 0;
	}

	mmc1.buffer = mmc1.shift = 0;
	mmc1.regs[0] = 0x1F;

	mmc1.regs[1] = 0;
	mmc1.regs[2] = 0;
	mmc1.regs[3] = 0;

	FixMMC1MIRRORING();
	FixMMC1CHR();
	FixMMC1PRG();
}

void GenMMC1Power(void) {
	lreset = 0;
	SetWriteHandler(0x8000, 0xFFFF, MMC1Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	if (WRAMSIZE) {
		FCEU_CheatAddRAM(8, 0x6000, WRAM);

		/* clear non-battery-backed portion of WRAM */
		if (NONSaveRAMSIZE) {
			FCEU_MemoryRand(WRAM, NONSaveRAMSIZE);
		}

		SetReadHandler(0x6000, 0x7FFF, MAWRAM);
		SetWriteHandler(0x6000, 0x7FFF, MBWRAM);
		setprg8r(0x10, 0x6000, 0);
	}

	MMC1RegReset();
}

void GenMMC1Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
	}
	WRAM = NULL;
}

void GenMMC1_Init(CartInfo *info, int wram, int saveram) {
	if ((info->mapper == 155) ||
		((info->mapper == 4) && (info->submapper == 3))) {
		mmc1_type = MMC1A;
	}

	mmc1.pwrap = GENPWRAP;
	mmc1.cwrap = GENCWRAP;
	mmc1.wwrap = GENWRAMWRAP;

	WRAMSIZE = wram * 1024;
	NONSaveRAMSIZE = (wram - saveram) * 1024;

	if (WRAMSIZE) {
		WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (saveram) {
			info->SaveGame[0] = WRAM + NONSaveRAMSIZE;
			info->SaveGameLen[0] = saveram * 1024;
		}
	}

	AddExState(mmc1.regs, 4, 0, "DREG");

	info->Power = GenMMC1Power;
	info->Close = GenMMC1Close;
	GameStateRestore = GenMMC1Restore;
	AddExState(&lreset, 8, 1, "LRST");
	AddExState(&mmc1.buffer, 1, 1, "BFFR");
	AddExState(&mmc1.shift, 1, 1, "BFRS");
}

void SAROM_Init(CartInfo *info) {
	GenMMC1_Init(info, 8, info->battery ? 8 : 0);
}

void SKROM_Init(CartInfo *info) {
	GenMMC1_Init(info, 8, info->battery ? 8 : 0);
}

void SNROM_Init(CartInfo *info) {
	GenMMC1_Init(info, 8, info->battery ? 8 : 0);
}

void SOROM_Init(CartInfo *info) {
	GenMMC1_Init(info, 16, info->battery ? 8 : 0);
}
