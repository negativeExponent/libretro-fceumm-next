/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2003 Xodnizel
 *  Mapper 12 code Copyright (C) 2003 CaH4e3
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

/*  Code for emulating iNES mappers 4,12,44,45,47,49,52,74,114,115,116,118,
 119,165,205,245,249,250,254
*/

#include "mapinc.h"
#include "mmc3.h"

MMC3 mmc3;

static uint8 *WRAM;
static uint32 WRAMSIZE;

static uint8 *CHRRAM;
static uint32 CHRRAMSIZE;

static uint8 IRQCount, IRQLatch, IRQa;
static uint8 IRQReload;
static uint8 submapper;

static SFORMAT MMC3_StateRegs[] =
{
	{ mmc3.regs, 8, "REGS" },
	{ &mmc3.cmd, 1, "CMD" },
	{ &mmc3.mirroring, 1, "A000" },
	{ &mmc3.wram, 1, "A001" },
	{ &IRQReload, 1, "IRQR" },
	{ &IRQCount, 1, "IRQC" },
	{ &IRQLatch, 1, "IRQL" },
	{ &IRQa, 1, "IRQA" },
	{ 0 }
};

int isRevB = 1;

void (*MMC3_FixPRG)(void);
void (*MMC3_FixCHR)(void);

void (*MMC3_pwrap)(uint32 A, uint8 V);
void (*MMC3_cwrap)(uint32 A, uint8 V);
void (*MMC3_mwrap)(uint8 V);

void GenMMC3_Init(CartInfo *info, int wram, int battery);

/* ----------------------------------------------------------------------
 * ------------------------- Generic MM3 Code ---------------------------
 * ----------------------------------------------------------------------
 */

uint8 MMC3GetPRGBank(int V) {
	if ((~V & 0x01) && (mmc3.cmd & 0x40)) {
		V ^= 0x02;
	}
	if (V & 0x02) {
		return ((~1) | (V & 0x01));
	}
	return mmc3.regs[6 | (V & 0x01)];
}

uint8 MMC3GetCHRBank(int V) {
	if (mmc3.cmd & 0x80) {
		V ^= 0x04;
	}
	if (V & 4) {
		return mmc3.regs[V - 2];
	}
	return ((mmc3.regs[V >> 1] & ~0x01) | (V & 0x01));
}

int MMC3CanWriteToWRAM(void) {
	return ((mmc3.wram & 0x80) && !(mmc3.wram & 0x40));
}

static void GenFixPRG(void) {
	MMC3_pwrap(0x8000, MMC3GetPRGBank(0));
	MMC3_pwrap(0xA000, MMC3GetPRGBank(1));
	MMC3_pwrap(0xC000, MMC3GetPRGBank(2));
	MMC3_pwrap(0xE000, MMC3GetPRGBank(3));
}

static void GenFixCHR(void) {
	MMC3_cwrap(0x0000, MMC3GetCHRBank(0));
	MMC3_cwrap(0x0400, MMC3GetCHRBank(1));
	MMC3_cwrap(0x0800, MMC3GetCHRBank(2));
	MMC3_cwrap(0x0C00, MMC3GetCHRBank(3));

	MMC3_cwrap(0x1000, MMC3GetCHRBank(4));
	MMC3_cwrap(0x1400, MMC3GetCHRBank(5));
	MMC3_cwrap(0x1800, MMC3GetCHRBank(6));
	MMC3_cwrap(0x1C00, MMC3GetCHRBank(7));

	if (MMC3_mwrap) {
		MMC3_mwrap(mmc3.mirroring);
	}
}

void MMC3RegReset(void) {
	IRQCount = IRQLatch = IRQa = mmc3.cmd = 0;
	mmc3.mirroring = mmc3.wram = 0;

	mmc3.regs[0] = 0;
	mmc3.regs[1] = 2;
	mmc3.regs[2] = 4;
	mmc3.regs[3] = 5;
	mmc3.regs[4] = 6;
	mmc3.regs[5] = 7;
	mmc3.regs[6] = 0;
	mmc3.regs[7] = 1;

	MMC3_FixPRG();
	MMC3_FixCHR();
}

DECLFW(MMC3_CMDWrite) {
	uint8 oldcmd = mmc3.cmd;
	/*	FCEU_printf("bs %04x %02x\n",A,V); */
	switch (A & 0xE001) {
		case 0x8000:
			mmc3.cmd = V;
			if ((oldcmd & 0x40) != (mmc3.cmd & 0x40))
				MMC3_FixPRG();
			if ((oldcmd & 0x80) != (mmc3.cmd & 0x80))
				MMC3_FixCHR();
			break;
		case 0x8001: {
			int cbase = (mmc3.cmd & 0x80) << 5;
			mmc3.regs[mmc3.cmd & 0x7] = V;
			switch (mmc3.cmd & 0x07) {
				case 0:
					MMC3_cwrap((cbase ^ 0x000), V & (~1));
					MMC3_cwrap((cbase ^ 0x400), V | 1);
					break;
				case 1:
					MMC3_cwrap((cbase ^ 0x800), V & (~1));
					MMC3_cwrap((cbase ^ 0xC00), V | 1);
					break;
				case 2:
					MMC3_cwrap(cbase ^ 0x1000, V);
					break;
				case 3:
					MMC3_cwrap(cbase ^ 0x1400, V);
					break;
				case 4:
					MMC3_cwrap(cbase ^ 0x1800, V);
					break;
				case 5:
					MMC3_cwrap(cbase ^ 0x1C00, V);
					break;
				case 6:
					if (mmc3.cmd & 0x40)
						MMC3_pwrap(0xC000, V);
					else
						MMC3_pwrap(0x8000, V);
					break;
				case 7:
					MMC3_pwrap(0xA000, V);
					break;
			}
			break;
		}
		case 0xA000:
			if (MMC3_mwrap)
				MMC3_mwrap(V);
			break;
		case 0xA001:
			mmc3.wram = V;
			break;
	}
}

DECLFW(MMC3_IRQWrite) {
	/*	FCEU_printf("%04x:%04x\n",A,V); */
	switch (A & 0xE001) {
		case 0xC000:
			IRQLatch = V;
			break;
		case 0xC001:
			IRQReload = 1;
			break;
		case 0xE000:
			X6502_IRQEnd(FCEU_IQEXT);
			IRQa = 0;
			break;
		case 0xE001:
			IRQa = 1;
			break;
	}
}

DECLFW(MMC3_Write) {
	/*	FCEU_printf("bs %04x %02x\n",A,V); */
	switch (A & 0xE000) {
	case 0x8000:
	case 0xA000:
		MMC3_CMDWrite(A, V);
		break;
	case 0xC000:
	case 0xE000:
		MMC3_IRQWrite(A, V);
		break;
	}
}

static void ClockMMC3Counter(void) {
	int count = IRQCount;
	if (!count || IRQReload) {
		IRQCount = IRQLatch;
		IRQReload = 0;
	} else
		IRQCount--;
	if ((count | isRevB) && !IRQCount) {
		if (IRQa) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void MMC3_hb(void) {
	ClockMMC3Counter();
}

static void MMC3_hb_KickMasterHack(void) {
	if (scanline == 238)
		ClockMMC3Counter();
	ClockMMC3Counter();
}

static void MMC3_hb_PALStarWarsHack(void) {
	if (scanline == 240)
		ClockMMC3Counter();
	ClockMMC3Counter();
}

void GenMMC3Restore(int version) {
	MMC3_FixPRG();
	MMC3_FixCHR();
}

static void GENCWRAP(uint32 A, uint8 V) {
	setchr1(A, V); /* Business Wars NEEDS THIS for 8K CHR-RAM */
}

static void GENPWRAP(uint32 A, uint8 V) {
	/* [NJ102] Mo Dao Jie (C) has 1024Mb MMC3 BOARD, maybe something other will be broken
	 * also HengGe BBC-2x boards enables this mode as default board mode at boot up
	 */
	setprg8(A, (V & 0x7F));
}

static void GENMWRAP(uint8 V) {
	mmc3.mirroring = V;
	setmirror((V & 1) ^ 1);
}

static void GENNOMWRAP(uint8 V) {
	mmc3.mirroring = V;
}

static DECLFW(MBWRAMMMC6) {
	WRAM[A & 0x3ff] = V;
}

static DECLFR(MAWRAMMMC6) {
	return (WRAM[A & 0x3ff]);
}

void GenMMC3Power(void) {
	if (UNIFchrrama)
		setchr8(0);

	SetWriteHandler(0x8000, 0xBFFF, MMC3_CMDWrite);
	SetWriteHandler(0xC000, 0xFFFF, MMC3_IRQWrite);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	mmc3.wram = mmc3.mirroring = 0;
	setmirror(1);
	if (mmc3.opts & 1) {
		if (WRAMSIZE == 1024) {
			FCEU_CheatAddRAM(1, 0x7000, WRAM);
			SetReadHandler(0x7000, 0x7FFF, MAWRAMMMC6);
			SetWriteHandler(0x7000, 0x7FFF, MBWRAMMMC6);
		} else {
			FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
			SetWriteHandler(0x6000, 0x6000 + ((WRAMSIZE - 1) & 0x1fff), CartBW);
			SetReadHandler(0x6000, 0x6000 + ((WRAMSIZE - 1) & 0x1fff), CartBR);
			setprg8r(0x10, 0x6000, 0);
		}
		if (!(mmc3.opts & 2))
			FCEU_MemoryRand(WRAM, WRAMSIZE);
	}
	MMC3RegReset();
	if (CHRRAM)
		FCEU_MemoryRand(CHRRAM, CHRRAMSIZE);
}

void GenMMC3Close(void) {
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	if (WRAM)
		FCEU_gfree(WRAM);
	CHRRAM = WRAM = NULL;
}

void GenMMC3_Init(CartInfo *info, int wram, int battery) {
	MMC3_FixPRG = GenFixPRG;
	MMC3_FixCHR = GenFixCHR;

	MMC3_pwrap = GENPWRAP;
	MMC3_cwrap = GENCWRAP;
	MMC3_mwrap = GENMWRAP;

	WRAMSIZE = wram << 10;

	if (wram) {
		mmc3.opts |= 1;
		WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}

	if (battery) {
		mmc3.opts |= 2;
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}

	AddExState(MMC3_StateRegs, ~0, 0, 0);

	info->Power = GenMMC3Power;
	info->Reset = MMC3RegReset;
	info->Close = GenMMC3Close;

	if (info->CRC32 == 0x5104833e) /* Kick Master */
		GameHBIRQHook = MMC3_hb_KickMasterHack;
	else if (info->CRC32 == 0x5a6860f1 || info->CRC32 == 0xae280e20) /* Shougi Meikan '92/'93 */
		GameHBIRQHook = MMC3_hb_KickMasterHack;
	else if (info->CRC32 == 0xfcd772eb) /* PAL Star Wars, similar problem as Kick Master. */
		GameHBIRQHook = MMC3_hb_PALStarWarsHack;
	else
		GameHBIRQHook = MMC3_hb;
	GameStateRestore = GenMMC3Restore;
	submapper = info->submapper;
}

/* ----------------------------------------------------------------------
 * -------------------------- MMC3 Based Code ---------------------------
 *  ----------------------------------------------------------------------
 */

/* ---------------------------- Mapper 4 -------------------------------- */

static int hackm4 = 0; /* For Karnov, maybe others.  BLAH.  Stupid iNES format.*/

static void M4Power(void) {
	GenMMC3Power();
	mmc3.mirroring = (hackm4 ^ 1) & 1;
	setmirror(hackm4);
}

void Mapper004_Init(CartInfo *info) {
	int ws = 8;

	if ((info->CRC32 == 0x93991433 || info->CRC32 == 0xaf65aa84)) {
		FCEU_printf("Low-G-Man can not work normally in the iNES format.\nThis game has been recognized by its CRC32 "
		            "value, and the appropriate changes will be made so it will run.\nIf you wish to hack this game, "
		            "you should use the UNIF format for your hack.\n\n");
		ws = 0;
	}
	if (info->CRC32 == 0x97b6cb19)
		isRevB = 0;

	GenMMC3_Init(info, ws, info->battery);
	info->Power = M4Power;
	hackm4 = info->mirror;
}

/* ---------------------------- Mapper 12 ------------------------------- */

static void M012CW(uint32 A, uint8 V) {
	setchr1(A, (mmc3.expregs[(A & 0x1000) >> 12] << 8) + V);
}

static DECLFW(M012Write) {
	mmc3.expregs[0] = V & 0x01;
	mmc3.expregs[1] = (V & 0x10) >> 4;
}

static DECLFR(M012Read) {
	return mmc3.expregs[2];
}

static void M012Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	mmc3.expregs[2] = 1; /* chinese is default */
	GenMMC3Power();
	SetWriteHandler(0x4100, 0x5FFF, M012Write);
	SetReadHandler(0x4100, 0x5FFF, M012Read);
}

static void M012Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	mmc3.expregs[2] ^= 1;
	MMC3RegReset();
}

void Mapper012_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M012CW;
	isRevB = 0;

	info->Power = M012Power;
	info->Reset = M012Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}

/* ---------------------------- Mapper 118 ------------------------------ */

static uint8 PPUCHRBus;
static uint8 TKSMIR[8];

static void FP_FASTAPASS(1) TKSPPU(uint32 A) {
	A &= 0x1FFF;
	A >>= 10;
	PPUCHRBus = A;
	setmirror(MI_0 + TKSMIR[A]);
}

static void TKSWRAP(uint32 A, uint8 V) {
	TKSMIR[A >> 10] = V >> 7;
	setchr1(A, V & 0x7F);
	if (PPUCHRBus == (A >> 10))
		setmirror(MI_0 + (V >> 7));
}

/* ---------------------------- UNIF Boards ----------------------------- */

void TBROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0); /* 64 - 64 */
}

void TEROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0); /* 32 - 32 */
}

void TFROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0); /* 512 - 64 */
}

void TGROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0); /* 512 - 0 */
}

void TKROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery); /* 512- 256 */
}

void TLROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0); /* 512 - 256 */
}

void TSROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0); /* 512 - 256 */
}

void TLSROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0); /* 512 - 256 */
	MMC3_cwrap = TKSWRAP;
	MMC3_mwrap = GENNOMWRAP;
	PPU_hook = TKSPPU;
	AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void TKSROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);  /* 512 - 256 */
	MMC3_cwrap = TKSWRAP;
	MMC3_mwrap = GENNOMWRAP;
	PPU_hook = TKSPPU;
	AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

static void TQWRAP(uint32 A, uint8 V) {
	setchr1r((V & 0x40) >> 2, A, V & 0x3F);
}

void TQROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);  /* 512 - 64 */
	MMC3_cwrap = TQWRAP;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

void HKROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 1, info->battery);  /* 512 - 512 */
}
