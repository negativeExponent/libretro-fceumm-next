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
 *
 * VRC-2/VRC-4 Konami
 * VRC-4 Pirate
 *
 */

#include "mapinc.h"
#include "vrcirq.h"
#include "eeprom_93C66.h"

static uint8 is22;
static uint16 IRQCount;
static uint8 IRQLatch, IRQa;
static uint8 prgreg[4], chrreg[8];
static uint16 chrhi[8];
static uint8 regcmd, mirr, big_bank;

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static uint8 prgMask = 0x1F;

static uint8 haveEEPROM;
static uint8 eeprom_data[256];

static SFORMAT StateRegs[] =
{
	{ prgreg, 4, "PREG" },
	{ chrreg, 8, "CREG" },
	{ chrhi, 16, "CHRH" },
	{ &regcmd, 1, "CMDR" },
	{ &mirr, 1, "MIRR" },
	{ &prgMask, 1, "MAK" },
	{ &big_bank, 1, "BIGB" },
	{ &IRQCount, 2, "IRQC" },
	{ &IRQLatch, 1, "IRQL" },
	{ &IRQa, 1, "IRQA" },
	{ 0 }
};

static void Sync(void) {
	if (regcmd & 2) {
		setprg8(0xC000, prgreg[0] | big_bank);
		setprg8(0x8000, (prgreg[2] & prgMask) | big_bank);
	} else {
		setprg8(0x8000, prgreg[0] | big_bank);
		setprg8(0xC000, (prgreg[2] & prgMask) | big_bank);
	}
	setprg8(0xA000, prgreg[1] | big_bank);
	setprg8(0xE000, (prgreg[3] & prgMask) | big_bank);
	if (UNIFchrrama)
		setchr8(0);
	else {
		uint8 i;
		for (i = 0; i < 8; i++)
			setchr1(i << 10, (chrhi[i] | chrreg[i]) >> is22);
	}
	switch (mirr & 0x3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

static DECLFW(VRC24Write) {
	A &= 0xF003;
	if ((A >= 0xB000) && (A <= 0xE003)) {
		if (UNIFchrrama)
			big_bank = (V & 8) << 2;							/* my personally many-in-one feature ;) just for support pirate cart 2-in-1 */
		else {
			uint16 i = ((A >> 1) & 1) | ((A - 0xB000) >> 11);
			uint16 nibble = ((A & 1) << 2);
			chrreg[i] = (chrreg[i] & (0xF0 >> nibble)) | ((V & 0xF) << nibble);
			if (nibble)
				chrhi[i] = (V & 0x10) << 4;						/* another one many in one feature from pirate carts */
		}
		Sync();
	} else {
		switch (A & 0xF003) {
		case 0x8000:
		case 0x8001:
		case 0x8002:
		case 0x8003:
			prgreg[0] = V & prgMask;
			Sync();
			break;
		case 0xA000:
		case 0xA001:
		case 0xA002:
		case 0xA003:
			prgreg[1] = V & prgMask;
			Sync();
			break;
		case 0x9000:
		case 0x9001: if (V != 0xFF) mirr = V; Sync(); break;
		case 0x9002:
		case 0x9003: regcmd = V; Sync(); break;
        case 0xF000: VRCIRQ_LatchNibble(V, 0); break;
		case 0xF001: VRCIRQ_LatchNibble(V, 1); break;
		case 0xF002: VRCIRQ_Control(V); break;
		case 0xF003: VRCIRQ_Acknowledge(); break;
        }
	}
}

static DECLFW(M21Write) {
	A = (A & 0xF000) | ((A >> 1) & 0x3) | ((A >> 6) & 0x3);		/* Ganbare Goemon Gaiden 2 - Tenka no Zaihou (J) [!] is Mapper 21*/
	VRC24Write(A, V);
}

static DECLFW(M22Write) {
#if 0
	/* Removed this hack, which was a bug in actual game cart.
	 * http://forums.nesdev.com/viewtopic.php?f=3&t=6584
	 */
	if ((A >= 0xC004) && (A <= 0xC007)) {						/* Ganbare Goemon Gaiden does strange things!!! at the end credits
		weirdo = 1;												 * quick dirty hack, seems there is no other games with such PCB, so
																 * we never know if it will not work for something else lol
																 */
	}
#endif
	A |= ((A >> 2) & 0x3);										/* It's just swapped lines from 21 mapper
																 */
	VRC24Write((A & 0xF000) | ((A >> 1) & 1) | ((A << 1) & 2), V);
}

static DECLFW(M23Write) {
	A |= ((A >> 2) & 0x3) | ((A >> 4) & 0x3);	/* actually there is many-in-one mapper source, some pirate or
												 * licensed games use various address bits for registers
												 */
	VRC24Write(A, V);
}

static void VRC24PowerCommon(void (*WRITEFUNC)(uint32 A, uint8 V)) {
	Sync();
	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);	/* Only two Goemon games are have battery backed RAM, three more shooters
									 * (Parodius Da!, Gradius 2 and Crisis Force uses 2k or SRAM at 6000-67FF only
									 */
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WRITEFUNC);
}

static void M21Power(void) {
	VRC24PowerCommon(M21Write);
}

static void M22Power(void) {
	VRC24PowerCommon(M22Write);
}

static void M23Power(void) {
	big_bank = 0x20;
	if ((prgreg[2] == 0x30) && (prgreg[3] == 0x31))
		big_bank = 0x00;
	VRC24PowerCommon(M23Write);
}

static void M25Power(void) {
	big_bank = 0x20;
	VRC24PowerCommon(M22Write);
}

static void StateRestore(int version) {
	Sync();
}

static void VRC24Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void VRC24_Init(CartInfo *info, uint32 hasWRAM) {
	info->Close = VRC24Close;
	GameStateRestore = StateRestore;

	VRCIRQ_Init();

	prgMask = 0x1F;
	prgreg[2] = ~1;
	prgreg[3] = ~0;

	WRAMSIZE = 0;

	/* 400K PRG + 128K CHR Contra rom hacks */
	if (info->PRGRomSize == 400 * 1024 && info->CHRRomSize == 128 * 1024) {
		prgreg[2] = 0x30;
		prgreg[3] = 0x31;
		prgMask = 0x3F;
	}

	if (hasWRAM) {
		WRAMSIZE = 8192;
		WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");

		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}

	AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper21_Init(CartInfo *info) {
	is22 = 0;
	info->Power = M21Power;
	VRC24_Init(info, 1);
}

void Mapper22_Init(CartInfo *info) {
	is22 = 1;
	info->Power = M22Power;
	VRC24_Init(info, 0);
}

void Mapper23_Init(CartInfo *info) {
	is22 = 0;
	info->Power = M23Power;
	VRC24_Init(info, 1);
}

void Mapper25_Init(CartInfo *info) {
	is22 = 0;
	info->Power = M25Power;
	VRC24_Init(info, 1);
}

/* -------------------- Mapper 529 -------------------- */
/* ------------------ UNIF UNL-T-230 ------------------ */

static void UNLT230PRGSync(void) {
	setprg16(0x8000, prgreg[1]);
	setprg16(0xC000, ~0);
}

static DECLFR(UNLT230EEPROMRead) {
	if (haveEEPROM) {
		return eeprom_93C66_read() ? 0x01 : 0x00;
	}
	return 0x01;
}

static DECLFW(UNLT230Write) {
	if (A & 0x800) {
		if (haveEEPROM) {
			eeprom_93C66_write(!!(A & 0x04), !!(A & 0x02), !!(A & 0x01));
		}
	} else {
		VRC24Write((A & 0xF000) | ((A >> 2) & 3), V);
		UNLT230PRGSync();
	}
}

static void UNLT230Power(void) {
	VRC24PowerCommon(M23Write);
	SetReadHandler(0x5000, 0x5FFF, UNLT230EEPROMRead);
	SetWriteHandler(0x8000, 0xFFFF, UNLT230Write);
}

static void UNLT230StateRestore(int version) {
	Sync();
	UNLT230PRGSync();
}

void UNLT230_Init(CartInfo *info) {
	is22 = 0;
	haveEEPROM = (info->PRGRamSaveSize & 0x100) != 0;

	VRC24_Init(info, !haveEEPROM);
	info->Power = UNLT230Power;
	GameStateRestore = UNLT230StateRestore;

	if (haveEEPROM) {
		eeprom_93C66_init(eeprom_data, 256, 16);
		info->battery = 1;
		info->SaveGame[0] = eeprom_data;
		info->SaveGameLen[0] = 256;
	}
}

/* -------------------- UNL-TH2131-1 -------------------- */
/* https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_308
 * NES 2.0 Mapper 308 is used for a bootleg version of the Sunsoft game Batman
 * similar to Mapper 23 Submapper 3) with custom IRQ functionality.
 * UNIF board name is UNL-TH2131-1.
 */

static DECLFW(TH2131Write) {
	switch (A & 0xF003) {
	case 0xF000: X6502_IRQEnd(FCEU_IQEXT); IRQa = 0; IRQCount = 0; break;
	case 0xF001: IRQa = 1; break;
	case 0xF003: IRQLatch = (V & 0xF0) >> 4; break;
	}
}

void FP_FASTAPASS(1) TH2131IRQHook(int a) {
	int count;

	if (!IRQa)
		return;

	for (count = 0; count < a; count++) {
		IRQCount++;
		if ((IRQCount & 0x0FFF) == 2048)
			IRQLatch--;
		if (!IRQLatch && (IRQCount & 0x0FFF) < 2048)
			X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void TH2131Power(void) {
	VRC24PowerCommon(VRC24Write);
	SetWriteHandler(0xF000, 0xFFFF, TH2131Write);
}

void UNLTH21311_Init(CartInfo *info) {
	info->Power = TH2131Power;
	VRC24_Init(info, 1);
	MapIRQHook = TH2131IRQHook;
}

/* -------------------- UNL-KS7021A -------------------- */
/* http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_525
 * NES 2.0 Mapper 525 is used for a bootleg version of versions of Contra and 月風魔伝 (Getsu Fūma Den).
 * Its similar to Mapper 23 Submapper 3) with non-nibblized CHR-ROM bank registers.
 */

static DECLFW(KS7021AWrite) {
	switch (A & 0xB000) {
	case 0xB000: chrreg[A & 0x07] = V; Sync(); break;
	}
}

static void KS7021APower(void) {
	VRC24PowerCommon(VRC24Write);
	SetWriteHandler(0xB000, 0xBFFF, KS7021AWrite);
}

void UNLKS7021A_Init(CartInfo *info) {
	info->Power = KS7021APower;
	VRC24_Init(info, 1);
}

/* -------------------- BTL-900218 -------------------- */
/* http://wiki.nesdev.com/w/index.php/UNIF/900218
 * NES 2.0 Mapper 524 describes the PCB used for the pirate port Lord of King or Axe of Fight.
 * UNIF board name is BTL-900218.
 */

static DECLFW(BTL900218Write) {
	switch (A & 0xF00C) {
	case 0xF008: IRQa = 1; break;
	case 0xF00C: X6502_IRQEnd(FCEU_IQEXT); IRQa = 0; IRQCount = 0; break;
	}
}

void FP_FASTAPASS(1) BTL900218IRQHook(int a) {
	if (!IRQa)
		return;

	IRQCount += a;
	if (IRQCount & 1024)
		X6502_IRQBegin(FCEU_IQEXT);
}

static void BTL900218Power(void) {
	VRC24PowerCommon(VRC24Write);
	SetWriteHandler(0xF000, 0xFFFF, BTL900218Write);
}

void BTL900218_Init(CartInfo *info) {
	info->Power = BTL900218Power;
	VRC24_Init(info, 1);
	MapIRQHook = BTL900218IRQHook;
}
