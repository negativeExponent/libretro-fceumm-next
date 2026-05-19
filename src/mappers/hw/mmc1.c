/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2023-2024-2026 negativeExponent
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
#include "mmc1.h"

/* size of non-battery-backed portion of WRAM */
/* serves as starting offset for actual save ram from total wram size */
/* returns 0 if entire work ram is battery backed ram */
static uint32_t NONSaveRAMSIZE = 0;

static MMC1TYPE type = MMC1B;

void (*MMC1_pwrap)(uint16_t A, uint16_t V);
void (*MMC1_cwrap)(uint16_t A, uint16_t V);

void (*MMC1_SyncPRG)(void);
void (*MMC1_SyncCHR)(void);
void (*MMC1_SyncMirror)(void);
void (*MMC1_SyncWRAM)(void);

MMC1 mmc1;

static SFORMAT StateRegs[] = {
	{ mmc1.reg, 4, "DREG" },
	{ &mmc1.buffer, 1, "BFFR" },
	{ &mmc1.shift, 1, "BFRS" },
	{ 0 }
};

uint32_t MMC1_GetPRGBank(int index) {
	uint32_t bank;
	uint8_t prg = mmc1.reg[3];

	switch (mmc1.reg[0] & 0xC) {
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

	if ((mmc1.reg[3] & 0x10) && (type == MMC1A)) {
		return ((bank & 0x07) | (mmc1.reg[3] & 0x08));
	}

	return (bank & 0x0F);
}

uint32_t MMC1_GetCHRBank(int index) {
	if (mmc1.reg[0] & 0x10) {
		return (mmc1.reg[1 + index]);
	}

	return ((mmc1.reg[1] & ~1) | index);
}

uint8_t MMC1_WRAMEnabled(void) {
	if ((mmc1.reg[3] & 0x10) && (type == MMC1B)) {
		return FALSE;
	}

	return TRUE;
}

void MMC1_pwrap_default(uint16_t A, uint16_t V) {
	setprg16(A, V & 0x0F);
}

void MMC1_cwrap_default(uint16_t A, uint16_t V) {
	setchr4(A, V & 0x1F);
}

void MMC1_SyncWRAM_default(void) {
	if (!WRAMSIZE || !MMC1_WRAMEnabled()) {
		unsetcpu8(0x6000);
	} else {
		setprg8r(0x10, 0x6000, 0);
	}
}

void MMC1_SyncPRG_default(void) {
	MMC1_pwrap(0x8000, MMC1_GetPRGBank(0));
	MMC1_pwrap(0xC000, MMC1_GetPRGBank(1));
}

void MMC1_SyncCHR_default(void) {
	MMC1_cwrap(0x0000, MMC1_GetCHRBank(0));
	MMC1_cwrap(0x1000, MMC1_GetCHRBank(1));
}

void MMC1_SyncMirror_default(void) {
	switch (mmc1.reg[0] & 3) {
	case 2:
		setmirror(MI_V);
		break;
	case 3:
		setmirror(MI_H);
		break;
	case 0:
		setmirror(MI_0);
		break;
	case 1:
		setmirror(MI_1);
		break;
	}
}

static uint64_t lreset;
DECLFW(MMC1_Write) {
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
		mmc1.reg[0] |= 0xC;
		mmc1.shift = mmc1.buffer = 0;
		MMC1_SyncPRG();
		lreset = timestampbase + timestamp;
		return;
	}

	mmc1.buffer |= (V & 1) << (mmc1.shift++);

	if (mmc1.shift == 5) {
		/* FCEU_printf("REG[%d]=%02x\n",n,mmc1.buffer); */
		mmc1.reg[n] = mmc1.buffer;
		mmc1.shift = mmc1.buffer = 0;
		switch (n) {
		case 0:
			MMC1_SyncMirror();
			MMC1_SyncCHR();
			MMC1_SyncPRG();
			break;
		case 1:
			MMC1_SyncCHR();
			MMC1_SyncPRG();
			MMC1_SyncWRAM();
			break;
		case 2:
			MMC1_SyncCHR();
			break;
		case 3:
			MMC1_SyncPRG();
			break;
		}
	}
}

void MMC1_Restore(int version) {
	MMC1_SyncMirror();
	MMC1_SyncCHR();
	MMC1_SyncPRG();
	MMC1_SyncWRAM();
	lreset = 0; /* timestamp(base) is not stored in save states. */
}

void MMC1_Reset(void) {
	mmc1.reg[0] = 0x0C;
	mmc1.reg[1] = 0;
	mmc1.reg[2] = 0;
	mmc1.reg[3] = 0;

	mmc1.buffer = mmc1.shift = 0;

	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
	MMC1_SyncWRAM();

	lreset = 0;
}

void MMC1_Power(void) {
	lreset = 0;
	SetWriteHandler(0x8000, 0xFFFF, MMC1_Write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	if (WRAMSIZE) {
		FCEU_CheatAddRAM(8, 0x6000, WRAM);

		/* clear non-battery-backed portion of WRAM */
		if (NONSaveRAMSIZE) {
			FCEU_MemoryRand(WRAM, NONSaveRAMSIZE);
		}

		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
	}

	MMC1_Reset();
}

void MMC1_Init(CartInfo *info, MMC1TYPE _type, int wram, int saveram) {
	MMC1_pwrap = MMC1_pwrap_default;
	MMC1_cwrap = MMC1_cwrap_default;
	MMC1_SyncPRG = MMC1_SyncPRG_default;
	MMC1_SyncCHR = MMC1_SyncCHR_default;
	MMC1_SyncWRAM = MMC1_SyncWRAM_default;
	MMC1_SyncMirror = MMC1_SyncMirror_default;

	WRAMSIZE = wram * 1024;
	NONSaveRAMSIZE = (wram - saveram) * 1024;

	type = _type;

	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (saveram) {
			info->SaveGame[0] = WRAM + NONSaveRAMSIZE;
			info->SaveGameLen[0] = saveram * 1024;
		}
	}

	info->Power = MMC1_Power;
	GameStateRestore = MMC1_Restore;

	AddExState(StateRegs, ~0, 0, NULL);
}

DECLFR(MMC1_readWRAM) {
	if (type == MMC1A || ~mmc1.reg[3] & 0x10) {
		return CartBR(A);
	}
	return A >> 8;
}

DECLFW(MMC1_writeWRAM) {
	if (type == MMC1A || ~mmc1.reg[3] & 0x10) {
		CartBW(A, V);
	}
}

void MMC1_SetConfig(uint8_t clear, MMC1TYPE _type) {
	type = _type;
	SetReadHandler (0x6000, 0x7FFF, MMC1_readWRAM);
	SetWriteHandler(0x6000, 0x7FFF, MMC1_writeWRAM);
	SetReadHandler (0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, MMC1_Write);
	if (clear) MMC1_Reset();
	else MMC1_Restore(0);
}

void SAROM_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 8, info->battery ? 8 : 0);
}

void SKROM_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 8, info->battery ? 8 : 0);
}

void SNROM_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 8, info->battery ? 8 : 0);
}

void SOROM_Init(CartInfo *info) {
	MMC1_Init(info, MMC1B, 16, info->battery ? 8 : 0);
}
