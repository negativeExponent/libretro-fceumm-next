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

uint8 *WRAM;
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

static int isRevB = 1;

void GenMMC3Power(void);
void FixMMC3PRG(int V);
void FixMMC3CHR(int V);

void GenMMC3_Init(CartInfo *info, int wram, int battery);

/* ----------------------------------------------------------------------
 * ------------------------- Generic MM3 Code ---------------------------
 * ----------------------------------------------------------------------
 */

int MMC3CanWriteToWRAM() {
	return ((mmc3.wram & 0x80) && !(mmc3.wram & 0x40));
}

void FixMMC3PRG(int V) {
	if (V & 0x40) {
		mmc3.pwrap(0xC000, mmc3.regs[6]);
		mmc3.pwrap(0x8000, ~1);
	} else {
		mmc3.pwrap(0x8000, mmc3.regs[6]);
		mmc3.pwrap(0xC000, ~1);
	}
	mmc3.pwrap(0xA000, mmc3.regs[7]);
	mmc3.pwrap(0xE000, ~0);
}

void FixMMC3CHR(int V) {
	int cbase = (V & 0x80) << 5;

	mmc3.cwrap((cbase ^ 0x000), mmc3.regs[0] & (~1));
	mmc3.cwrap((cbase ^ 0x400), mmc3.regs[0] | 1);
	mmc3.cwrap((cbase ^ 0x800), mmc3.regs[1] & (~1));
	mmc3.cwrap((cbase ^ 0xC00), mmc3.regs[1] | 1);

	mmc3.cwrap(cbase ^ 0x1000, mmc3.regs[2]);
	mmc3.cwrap(cbase ^ 0x1400, mmc3.regs[3]);
	mmc3.cwrap(cbase ^ 0x1800, mmc3.regs[4]);
	mmc3.cwrap(cbase ^ 0x1c00, mmc3.regs[5]);

	if (mmc3.mwrap)
		mmc3.mwrap(mmc3.mirroring);
}

void MMC3RegReset(void) {
	IRQCount = IRQLatch = IRQa = mmc3.cmd = 0;

	mmc3.regs[0] = 0;
	mmc3.regs[1] = 2;
	mmc3.regs[2] = 4;
	mmc3.regs[3] = 5;
	mmc3.regs[4] = 6;
	mmc3.regs[5] = 7;
	mmc3.regs[6] = 0;
	mmc3.regs[7] = 1;

	FixMMC3PRG(0);
	FixMMC3CHR(0);
}

DECLFW(MMC3_CMDWrite) {
	/*	FCEU_printf("bs %04x %02x\n",A,V); */
	switch (A & 0xE001) {
		case 0x8000:
			if ((V & 0x40) != (mmc3.cmd & 0x40))
				FixMMC3PRG(V);
			if ((V & 0x80) != (mmc3.cmd & 0x80))
				FixMMC3CHR(V);
			mmc3.cmd = V;
			break;
		case 0x8001: {
			int cbase = (mmc3.cmd & 0x80) << 5;
			mmc3.regs[mmc3.cmd & 0x7] = V;
			switch (mmc3.cmd & 0x07) {
				case 0:
					mmc3.cwrap((cbase ^ 0x000), V & (~1));
					mmc3.cwrap((cbase ^ 0x400), V | 1);
					break;
				case 1:
					mmc3.cwrap((cbase ^ 0x800), V & (~1));
					mmc3.cwrap((cbase ^ 0xC00), V | 1);
					break;
				case 2:
					mmc3.cwrap(cbase ^ 0x1000, V);
					break;
				case 3:
					mmc3.cwrap(cbase ^ 0x1400, V);
					break;
				case 4:
					mmc3.cwrap(cbase ^ 0x1800, V);
					break;
				case 5:
					mmc3.cwrap(cbase ^ 0x1C00, V);
					break;
				case 6:
					if (mmc3.cmd & 0x40)
						mmc3.pwrap(0xC000, V);
					else
						mmc3.pwrap(0x8000, V);
					break;
				case 7:
					mmc3.pwrap(0xA000, V);
					break;
			}
			break;
		}
		case 0xA000:
			if (mmc3.mwrap)
				mmc3.mwrap(V);
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
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
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
	mmc3.pwrap = GENPWRAP;
	mmc3.cwrap = GENCWRAP;
	mmc3.mwrap = GENMWRAP;

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
	mmc3.cwrap = M012CW;
	isRevB = 0;

	info->Power = M012Power;
	info->Reset = M012Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}

/* ---------------------------- Mapper 37 ------------------------------- */

static void M037PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] != 2)
		V &= 0x7;
	else
		V &= 0xF;
	V |= mmc3.expregs[0] << 3;
	setprg8(A, V);
}

static void M037CW(uint32 A, uint8 V) {
	uint32 NV = V;
	NV &= 0x7F;
	NV |= mmc3.expregs[0] << 6;
	setchr1(A, NV);
}

static DECLFW(M037Write) {
	mmc3.expregs[0] = (V & 6) >> 1;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M037Reset(void) {
	mmc3.expregs[0] = 0;
	MMC3RegReset();
}

static void M037Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M037Write);
}

void Mapper037_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.pwrap = M037PW;
	mmc3.cwrap = M037CW;
	info->Power = M037Power;
	info->Reset = M037Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

/* ---------------------------- Mapper 44 ------------------------------- */

static void M044PW(uint32 A, uint8 V) {
	uint32 NV = V;
	if (mmc3.expregs[0] >= 6)
		NV &= 0x1F;
	else
		NV &= 0x0F;
	NV |= mmc3.expregs[0] << 4;
	setprg8(A, NV);
}

static void M044CW(uint32 A, uint8 V) {
	uint32 NV = V;
	if (mmc3.expregs[0] < 6)
		NV &= 0x7F;
	NV |= mmc3.expregs[0] << 7;
	setchr1(A, NV);
}

static DECLFW(M044Write) {
	if (A & 1) {
		mmc3.expregs[0] = V & 7;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	} else
		MMC3_CMDWrite(A, V);
}

static void M044Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0xA000, 0xBFFF, M044Write);
}

void Mapper044_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M044CW;
	mmc3.pwrap = M044PW;
	info->Power = M044Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

/* ---------------------------- Mapper 45 ------------------------------- */

static void M045CW(uint32 A, uint8 V) {
	if (UNIFchrrama) {
		/* assume chr-ram, 4-in-1 Yhc-Sxx-xx variants */
		setchr1(A, V);
	} else {
		uint32 mask = 0xFF >> (~mmc3.expregs[2] & 0xF);
		uint32 base = ((mmc3.expregs[2] << 4) & 0xF00) | mmc3.expregs[0];
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static DECLFR(M045ReadOB) {
	return X.DB;
}

static void M045PW(uint32 A, uint8 V) {
	uint32 mask = ~mmc3.expregs[3] & 0x3F;
	uint32 base = ((mmc3.expregs[2] << 2) & 0x300) | mmc3.expregs[1];
	setprg8(A, (base & ~mask) | (V & mask));

	/* Some multicarts select between five different menus by connecting one of the higher address lines to PRG /CE.
	   The menu code selects between menus by checking which of the higher address lines disables PRG-ROM when set. */
	if ((PRGsize[0] < 0x200000 && mmc3.expregs[5] == 1 && (mmc3.expregs[1] & 0x80)) ||
	    (PRGsize[0] < 0x200000 && mmc3.expregs[5] == 2 && (mmc3.expregs[2] & 0x40)) ||
	    (PRGsize[0] < 0x100000 && mmc3.expregs[5] == 3 && (mmc3.expregs[1] & 0x40)) ||
	    (PRGsize[0] < 0x100000 && mmc3.expregs[5] == 4 && (mmc3.expregs[2] & 0x20))) {
		SetReadHandler(0x8000, 0xFFFF, M045ReadOB); 
	} else {
		SetReadHandler(0x8000, 0xFFFF, CartBR);
	}
}

static DECLFW(M045Write) {
	CartBW(A, V);
	if (!(mmc3.expregs[3] & 0x40)) {
		mmc3.expregs[mmc3.expregs[4]] = V;
		mmc3.expregs[4] = (mmc3.expregs[4] + 1) & 3;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static DECLFR(M045Read) {
	uint32 addr = 1 << (mmc3.expregs[5] + 4);
	if (A & (addr | (addr - 1)))
		return X.DB | 1;
	else
		return X.DB;
}

static void M045Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = 0;
	mmc3.expregs[2] = 0x0F;
	mmc3.expregs[5]++;
	mmc3.expregs[5] &= 7;
	MMC3RegReset();
}

static void M045Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[3] = mmc3.expregs[4] = mmc3.expregs[5] = 0;
	mmc3.expregs[2] = 0x0F;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M045Write);
	SetReadHandler(0x5000, 0x5FFF, M045Read);
}

void Mapper045_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M045CW;
	mmc3.pwrap = M045PW;
	info->Reset = M045Reset;
	info->Power = M045Power;
	AddExState(mmc3.expregs, 5, 0, "EXPR");
}

/* ---------------------------- Mapper 47 ------------------------------- */

static void M047PW(uint32 A, uint8 V) {
	V &= 0xF;
	V |= mmc3.expregs[0] << 4;
	setprg8(A, V);
}

static void M047CW(uint32 A, uint8 V) {
	uint32 NV = V;
	NV &= 0x7F;
	NV |= mmc3.expregs[0] << 7;
	setchr1(A, NV);
}

static DECLFW(M047Write) {
	mmc3.expregs[0] = V & 1;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M047Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M047Write);
	/*	SetReadHandler(0x6000,0x7FFF,0); */
}

void Mapper047_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = M047PW;
	mmc3.cwrap = M047CW;
	info->Power = M047Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

/* ---------------------------- Mapper 49 ------------------------------- */
/* -------------------- BMC-STREETFIGTER-GAME4IN1 ----------------------- */
/* added 6-24-19:
 * BMC-STREETFIGTER-GAME4IN1 - Sic. $6000 set to $41 rather than $00 on power-up.
 */

static void M49PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 1) {
		setprg8(A, ((mmc3.expregs[0] >> 2) & ~0x0F) | (V & 0x0F));
	} else {
		setprg32(0x8000, (mmc3.expregs[0] >> 4) & 3);
	}
}

static void M49CW(uint32 A, uint8 V) {
	setchr1(A, ((mmc3.expregs[0] << 1) & 0x180) | (V & 0x7F));
}

static DECLFW(M49Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}
}

static void M49Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1];
	MMC3RegReset();
}

static void M49Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1];
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M49Write);
}

void Mapper049_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = M49CW;
	mmc3.pwrap = M49PW;
	info->Reset = M49Reset;
	info->Power = M49Power;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
	mmc3.expregs[1] = 0;
	if (info->PRGCRC32 == 0x408EA235)
		mmc3.expregs[1] = 0x41; /* Street Fighter II Game 4-in-1 */
}

/* ---------------------------- Mapper 52 ------------------------------- */
/* Submapper 13 - CHR-ROM + CHR-RAM */
static void M052PW(uint32 A, uint8 V) {
	uint32 mask = (mmc3.expregs[0] & 8) ? 0x0F : 0x1F;
	setprg8(A, ((mmc3.expregs[0] << 4) & 0x70) | (V & mask));
}

static void M052CW(uint32 A, uint8 V) {
	uint32 mask = (mmc3.expregs[0] & 0x40) ? 0x7F : 0xFF;
	uint32 bank = (submapper == 14) ? (((mmc3.expregs[0] << 3) & 0x080) | ((mmc3.expregs[0] << 7) & 0x300)) :
		(((mmc3.expregs[0] << 3) & 0x180) | ((mmc3.expregs[0] << 7) & 0x200));
	uint8 ram = CHRRAM && (
		((submapper == 13) && ((mmc3.expregs[0] & 3) == 0x3)) ||
		((submapper == 14) && (mmc3.expregs[0] & 0x20)));
	if (ram) {
		setchr8r(0x10, 0);
	} else {
		setchr1(A, bank | (V & mask));
	}
}

static DECLFW(M052Write) {
	if (mmc3.expregs[1]) {
		WRAM[A - 0x6000] = V;
		return;
	}
	mmc3.expregs[1] = V & 0x80;
	mmc3.expregs[0] = V;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M052Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

static void M052Power(void) {
	M052Reset();
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M052Write);
}

void Mapper052_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M052CW;
	mmc3.pwrap = M052PW;
	info->Reset = M052Reset;
	info->Power = M052Power;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
	if (info->iNES2 && ((info->submapper == 13) || (info->submapper == 14))) {
		CHRRAMSIZE = info->CHRRamSize ? info->CHRRamSize : 8192;
		CHRRAM     = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
	}
	if (info->CRC32 == 0xCCE8CA2F && submapper != 14) {
		/* Well 8-in-1 (AB128) (Unl) (p1), with 1024 PRG and CHR is incompatible with submapper 13.
		 * This was reassigned to submapper 14 instead. */
		submapper = 14;
	}
}

/* ---------------------------- Mapper 76 ------------------------------- */

static void M076CW(uint32 A, uint8 V) {
	if (A >= 0x1000)
		setchr2((A & 0xC00) << 1, V);
}

void Mapper076_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = M076CW;
}

/* ---------------------------- Mapper 74 ------------------------------- */

static void M074CW(uint32 A, uint8 V) {
	if ((V == 8) || (V == 9)) /* Di 4 Ci - Ji Qi Ren Dai Zhan (As).nes, Ji Jia Zhan Shi (As).nes */
		setchr1r(0x10, A, V);
	else
		setchr1r(0, A, V);
}

void Mapper074_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M074CW;
	CHRRAMSIZE = 2048;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

/* ---------------------------- Mapper 114 ------------------------------ */

static uint8 cmdin, type_Boogerman = 0;
uint8 boogerman_perm[8] = { 0, 2, 5, 3, 6, 1, 7, 4 };
uint8 m114_perm[8] = { 0, 3, 1, 5, 6, 7, 2, 4 };

static void M114PWRAP(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x80) {
		if (mmc3.expregs[0] & 0x20)
			setprg32(0x8000, (mmc3.expregs[0] & 0x0F) >> 1);
		else {
			setprg16(0x8000, (mmc3.expregs[0] & 0x0F));
			setprg16(0xC000, (mmc3.expregs[0] & 0x0F));
		}
	} else
		setprg8(A, V);
}

static void M114CWRAP(uint32 A, uint8 V) {
	setchr1(A, (uint32)V | ((mmc3.expregs[1] & 1) << 8));
}

static DECLFW(M114Write) {
	switch (A & 0xE001) {
		case 0x8001:
			MMC3_CMDWrite(0xA000, V);
			break;
		case 0xA000:
			MMC3_CMDWrite(0x8000, (V & 0xC0) | (m114_perm[V & 7]));
			cmdin = 1;
			break;
		case 0xC000:
			if (!cmdin)
				break;
			MMC3_CMDWrite(0x8001, V);
			cmdin = 0;
			break;
		case 0xA001:
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

static DECLFW(BoogermanWrite) {
	switch (A & 0xE001) {
		case 0x8001:
			if (!cmdin)
				break;
			MMC3_CMDWrite(0x8001, V);
			cmdin = 0;
			break;
		case 0xA000:
			MMC3_CMDWrite(0x8000, (V & 0xC0) | (boogerman_perm[V & 7]));
			cmdin = 1;
			break;
		case 0xA001:
			IRQReload = 1;
			break;
		case 0xC000:
			MMC3_CMDWrite(0xA000, V);
			break;
		case 0xC001:
			IRQLatch = V;
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

static DECLFW(M114ExWrite) {
	if (A <= 0x7FFF) {
		if (A & 1)
			mmc3.expregs[1] = V;
		else
			mmc3.expregs[0] = V;
		FixMMC3PRG(mmc3.cmd);
	}
}

static void M114Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M114ExWrite);
	SetWriteHandler(0x8000, 0xFFFF, M114Write);
	if (type_Boogerman)
		SetWriteHandler(0x8000, 0xFFFF, BoogermanWrite);
}

static void M114Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	MMC3RegReset();
}

void Mapper114_Init(CartInfo *info) {
	isRevB = 0;
	/* Use NES 2.0 submapper to identify scrambling pattern, otherwise CRC32 for Boogerman and test rom */
	type_Boogerman = info->iNES2 ? (info->submapper == 1) : (info->CRC32 == 0x80eb1839 || info->CRC32 == 0x071e4ee8);

	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M114PWRAP;
	mmc3.cwrap = M114CWRAP;
	info->Power = M114Power;
	info->Reset = M114Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
	AddExState(&cmdin, 1, 0, "CMDI");
}

/* ---------------------------- Mapper 115 KN-658 board ------------------------------ */

static void M115PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x80) {
		if (mmc3.expregs[0] & 0x20)
			setprg32(0x8000, (mmc3.expregs[0] & 0x0F) >> 1); /* real hardware tests, info 100% now lol */
		else {
			setprg16(0x8000, (mmc3.expregs[0] & 0x0F));
			setprg16(0xC000, (mmc3.expregs[0] & 0x0F));
		}
	} else
		setprg8(A, V);
}

static void M115CW(uint32 A, uint8 V) {
	setchr1(A, (uint32)V | ((mmc3.expregs[1] & 1) << 8));
}

static DECLFW(M115Write) {
	if (A == 0x5080)
		mmc3.expregs[2] = V; /* Extra prot hardware 2-in-1 mode */
	else if (A == 0x6000)
		mmc3.expregs[0] = V;
	else if (A == 0x6001)
		mmc3.expregs[1] = V;
	FixMMC3PRG(mmc3.cmd);
}

static DECLFR(M115Read) {
	return mmc3.expregs[2];
}

static void M115Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x4100, 0x7FFF, M115Write);
	SetReadHandler(0x5000, 0x5FFF, M115Read);
}

void Mapper115_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = M115CW;
	mmc3.pwrap = M115PW;
	info->Power = M115Power;
	AddExState(mmc3.expregs, 3, 0, "EXPR");
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

/* ---------------------------- Mapper 119 ------------------------------ */

static void TQWRAP(uint32 A, uint8 V) {
	setchr1r((V & 0x40) >> 2, A, V & 0x3F);
}

void Mapper119_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap = TQWRAP;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

/* ---------------------------- Mapper 165 ------------------------------ */

static void M165CW(uint32 A, uint8 V) {
	if (V == 0)
		setchr4r(0x10, A, 0);
	else
		setchr4(A, V >> 2);
}

static void M165PPUFD(void) {
	if (mmc3.expregs[0] == 0xFD) {
		M165CW(0x0000, mmc3.regs[0]);
		M165CW(0x1000, mmc3.regs[2]);
	}
}

static void M165PPUFE(void) {
	if (mmc3.expregs[0] == 0xFE) {
		M165CW(0x0000, mmc3.regs[1]);
		M165CW(0x1000, mmc3.regs[4]);
	}
}

static void M165CWM(uint32 A, uint8 V) {
	if (((mmc3.cmd & 0x7) == 0) || ((mmc3.cmd & 0x7) == 2))
		M165PPUFD();
	if (((mmc3.cmd & 0x7) == 1) || ((mmc3.cmd & 0x7) == 4))
		M165PPUFE();
}

static void FP_FASTAPASS(1) M165PPU(uint32 A) {
	if ((A & 0x1FF0) == 0x1FD0) {
		mmc3.expregs[0] = 0xFD;
		M165PPUFD();
	} else if ((A & 0x1FF0) == 0x1FE0) {
		mmc3.expregs[0] = 0xFE;
		M165PPUFE();
	}
}

static void M165Power(void) {
	mmc3.expregs[0] = 0xFD;
	GenMMC3Power();
}

void Mapper165_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M165CWM;
	PPU_hook = M165PPU;
	info->Power = M165Power;
	CHRRAMSIZE = 4096;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
	AddExState(mmc3.expregs, 4, 0, "EXPR");
}

/* ---------------------------- Mapper 191 ------------------------------ */

static void M191CW(uint32 A, uint8 V) {
	setchr1r((V & 0x80) >> 3, A, V);
}

void Mapper191_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M191CW;
	CHRRAMSIZE = 2048;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

/* ---------------------------- Mapper 192 ------------------------------- */

static void M192CW(uint32 A, uint8 V) {
	/* Ying Lie Qun Xia Zhuan (Chinese),
	 * You Ling Xing Dong (China) (Unl) [this will be mistakenly headered as m074 sometimes]
	 */
	if ((V == 8) || (V == 9) || (V == 0xA) || (V == 0xB))
		setchr1r(0x10, A, V);
	else
		setchr1r(0, A, V);
}

void Mapper192_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M192CW;
	CHRRAMSIZE = 4096;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

/* ---------------------------- Mapper 194 ------------------------------- */

static void M194CW(uint32 A, uint8 V) {
	if (V <= 1) /* Dai-2-Ji - Super Robot Taisen (As).nes */
		setchr1r(0x10, A, V);
	else
		setchr1r(0, A, V);
}

void Mapper194_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M194CW;
	CHRRAMSIZE = 2048;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

/* ---------------------------- Mapper 196 ------------------------------- */
/* MMC3 board with optional command address line connection, allows to
 * make three-four different wirings to IRQ address lines and separately to
 * CMD address line, Mali Boss additionally check if wiring are correct for
 * game
 */

static void M196PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0])
		setprg32(0x8000, mmc3.expregs[1]);
	else
		setprg8(A, V);
}

static DECLFW(Mapper196Write) {
	A = (A & 0xF000) | (!!(A & 0xE) ^ (A & 1));
	if (A >= 0xC000)
		MMC3_IRQWrite(A, V);
	else
		MMC3_CMDWrite(A, V);
}

static DECLFW(Mapper196WriteLo) {
	mmc3.expregs[0] = 1;
	mmc3.expregs[1] = (V & 0xf) | (V >> 4);
	FixMMC3PRG(mmc3.cmd);
}

static void Mapper196Power(void) {
	GenMMC3Power();
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	SetWriteHandler(0x6000, 0x6FFF, Mapper196WriteLo);
	SetWriteHandler(0x8000, 0xFFFF, Mapper196Write);
}

void Mapper196_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M196PW;
	info->Power = Mapper196Power;
}

/* ---------------------------- Mali Splash Bomb---------------------------- */
/* The same board as for 196 mapper games, but with additional data bit swap
 * Also, it is impossible to work on the combined 196 mapper source with
 * all data bits merged, because it's using one of them as 8000 reg...
 */

static void M325PW(uint32 A, uint8 V) {
	setprg8(A, (V & 3) | ((V & 8) >> 1) | ((V & 4) << 1));
}

static void M325CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0xDD) | ((V & 0x20) >> 4) | ((V & 2) << 4));
}

static DECLFW(M325Write) {
	if (A >= 0xC000) {
		A = (A & 0xFFFE) | ((A >> 2) & 1) | ((A >> 3) & 1);
		MMC3_IRQWrite(A, V);
	} else {
		A = (A & 0xFFFE) | ((A >> 3) & 1);
		MMC3_CMDWrite(A, V);
	}
}

static void M325Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x8000, 0xFFFF, M325Write);
}

void Mapper325_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = M325PW;
	mmc3.cwrap = M325CW;
	info->Power = M325Power;
}

/* ---------------------------- Mapper 197 ------------------------------- */

static void M197CW(uint32 A, uint8 V) {
	if (A == 0x0000)
		setchr4(0x0000, V >> 1);
	else if (A == 0x1000)
		setchr2(0x1000, V);
	else if (A == 0x1400)
		setchr2(0x1800, V);
}

void Mapper197_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.cwrap = M197CW;
}

/* ---------------------------- Mapper 198 ------------------------------- */

static void M198PW(uint32 A, uint8 V) {
	if (V >= 0x50) /* Tenchi o Kurau II - Shokatsu Koumei Den (J) (C).nes */
		setprg8(A, V & 0x4F);
	else
		setprg8(A, V);
}

static void M198Power(void) {
	GenMMC3Power();
	setprg4r(0x10, 0x5000, 2);
	SetWriteHandler(0x5000, 0x5fff, CartBW);
	SetReadHandler(0x5000, 0x5fff, CartBR);
}

void Mapper198_Init(CartInfo *info) {
	GenMMC3_Init(info, 16, info->battery);
	mmc3.pwrap = M198PW;
	info->Power = M198Power;
}

/* ---------------------------- Mapper 205 ------------------------------ */
/* UNIF boardname BMC-JC-016-2
https://wiki.nesdev.com/w/index.php/INES_Mapper_205 */

/* 2023-02 : Update reg write logic and add solder pad */

static void M205PW(uint32 A, uint8 V) {
	uint8 bank = V & ((mmc3.expregs[0] & 0x02) ? 0x0F : 0x1F);
	setprg8(A, mmc3.expregs[0] << 4 | bank);
}

static void M205CW(uint32 A, uint8 V) {
	uint8 bank = V & ((mmc3.expregs[0] & 0x02) ? 0x7F : 0xFF);
	setchr1(A, (mmc3.expregs[0] << 7) | bank);
}

static DECLFW(M205Write) {
	mmc3.expregs[0] = V & 3;
	if (V & 1) {
		mmc3.expregs[0] |= mmc3.expregs[1];
	}
	CartBW(A, V);
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M205Reset(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] ^= 2; /* solder pad */
	MMC3RegReset();
}

static void M205Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M205Write);
}

void Mapper205_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = M205PW;
	mmc3.cwrap = M205CW;
	info->Power = M205Power;
	info->Reset = M205Reset;
	AddExState(mmc3.expregs, 2, 0, "EXPR");
}

/* --------------------------- GN-45 BOARD ------------------------------ */

/* Mapper 361 and 366, previously assigned as Mapper 205 */
/* Mapper 361:
 *  JY-009
 *  JY-018
 *  JY-019
 *  OK-411
 * Mapper 366 (GN-45):
 *  K-3131GS
 *  K-3131SS
*/	
static void GN45PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x0f) | mmc3.expregs[0]);
}

static void GN45CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | (mmc3.expregs[0] << 3));
}

static void GN45Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[2] = 0;
	MMC3RegReset();
}

static DECLFW(M361Write) {
	mmc3.expregs[0] = V & 0xF0;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M361Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x7000, 0x7fff, M361Write); /* OK-411 boards, the same logic, but data latched, 2-in-1 frankenstein */
}

void Mapper361_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.pwrap = GN45PW;
	mmc3.cwrap = GN45CW;
	info->Power = M361Power;
	info->Reset = GN45Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

static DECLFW(M366Write) {
	CartBW(A, V);
	if (mmc3.expregs[2] == 0) {
		mmc3.expregs[0] = A & 0x70;
		mmc3.expregs[2] = A & 0x80;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	}	
}

static void M366Power(void) {
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7fff, M366Write);
}

void Mapper366_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, 0);
	mmc3.pwrap = GN45PW;
	mmc3.cwrap = GN45CW;
	info->Power = M366Power;
	info->Reset = GN45Reset;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

/* ---------------------------- Mapper 245 ------------------------------ */

static void M245CW(uint32 A, uint8 V) {
	if (!UNIFchrrama) /* Yong Zhe Dou E Long - Dragon Quest VI (As).nes NEEDS THIS for RAM cart */
		setchr1(A, V & 7);
	mmc3.expregs[0] = V;
	FixMMC3PRG(mmc3.cmd);
}

static void M245PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x3F) | ((mmc3.expregs[0] & 2) << 5));
}

static void M245Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
}

void Mapper245_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M245CW;
	mmc3.pwrap = M245PW;
	info->Power = M245Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
}

/* ---------------------------- Mapper 249 ------------------------------ */

static void M249PW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x2) {
		if (V < 0x20)
			V = (V & 1) | ((V >> 3) & 2) | ((V >> 1) & 4) | ((V << 2) & 8) | ((V << 2) & 0x10);
		else {
			V -= 0x20;
			V = (V & 3) | ((V >> 1) & 4) | ((V >> 4) & 8) | ((V >> 2) & 0x10) | ((V << 3) & 0x20) | ((V << 2) & 0xC0);
		}
	}
	setprg8(A, V);
}

static void M249CW(uint32 A, uint8 V) {
	if (mmc3.expregs[0] & 0x2)
		V = (V & 3) | ((V >> 1) & 4) | ((V >> 4) & 8) | ((V >> 2) & 0x10) | ((V << 3) & 0x20) | ((V << 2) & 0xC0);
	setchr1(A, V);
}

static DECLFW(M249Write) {
	mmc3.expregs[0] = V;
	FixMMC3PRG(mmc3.cmd);
	FixMMC3CHR(mmc3.cmd);
}

static void M249Power(void) {
	mmc3.expregs[0] = 0;
	GenMMC3Power();
	SetWriteHandler(0x5000, 0x5000, M249Write);
}

void Mapper249_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = M249CW;
	mmc3.pwrap = M249PW;
	info->Power = M249Power;
	AddExState(mmc3.expregs, 1, 0, "EXPR");
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
	mmc3.cwrap = TKSWRAP;
	mmc3.mwrap = GENNOMWRAP;
	PPU_hook = TKSPPU;
	AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void TKSROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);  /* 512 - 256 */
	mmc3.cwrap = TKSWRAP;
	mmc3.mwrap = GENNOMWRAP;
	PPU_hook = TKSPPU;
	AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void TQROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);  /* 512 - 64 */
	mmc3.cwrap = TQWRAP;
	CHRRAMSIZE = 8192;
	CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");
}

void HKROM_Init(CartInfo *info) {
	GenMMC3_Init(info, 1, info->battery);  /* 512 - 512 */
}
