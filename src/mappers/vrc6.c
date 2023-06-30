/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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
 * VRC-6
 *
 */

#include "mapinc.h"
#include "vrc6.h"
#include "vrcirq.h"
#include "vrc6sound.h"

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static uint32 vrc6_A0;
static uint32 vrc6_A1;

VRC6 vrc6;

static SFORMAT StateRegs[] =
{
	{ vrc6.prg, 2, "PRG" },
	{ vrc6.chr, 8, "CHR" },
	{ &vrc6.mirr, 1, "MIRR" },

	{ 0 }
};

static void GENPWRAP(uint32 A, uint8 V) {
	setprg8(A, V & 0x3F);
}

static void GENCWRAP(uint32 A, uint32 V) {
	setchr1(A, V & 0x1FF);
}

static void GENMWRAP(uint8 V) {
	vrc6.mirr = V;

	switch (vrc6.mirr & 3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

void FixVRC6PRG(void) {
	vrc6.pwrap(0x8000, (vrc6.prg[0] << 1) | 0x00);
	vrc6.pwrap(0xa000, (vrc6.prg[0] << 1) | 0x01);
	vrc6.pwrap(0xc000, vrc6.prg[1]);
	vrc6.pwrap(0xe000, ~0);
}

void FixVRC6CHR(void) {
	int i;
	for (i = 0; i < 8; i++) {
		vrc6.cwrap(i << 10, vrc6.chr[i]);
	}

	if (vrc6.mwrap) {
		vrc6.mwrap(vrc6.mirr);
	}
}

DECLFW(VRC6Write) {
	int index;

	A = (A & 0xF000) | ((A & vrc6_A1) ? 0x02 : 0x00) | ((A & vrc6_A0) ? 0x01 : 0x00);
	switch (A & 0xF000) {
	case 0x8000:
		vrc6.prg[0] = V;
		vrc6.pwrap(0x8000, (V << 1) | 0x00);
		vrc6.pwrap(0xA000, (V << 1) | 0x01);
		break;
	case 0x9000:
	case 0xA000:
	case 0xB000:
		if (A >= 0x9000 && A <= 0xB002) {
			VRC6SW(A, V);
		} else {
			vrc6.mirr = (V >> 2) & 3;
			if (vrc6.mwrap) {
				vrc6.mwrap(vrc6.mirr);
			}
		}
		break;
	case 0xC000:
		vrc6.prg[1] = V;
		vrc6.pwrap(0xC000, V);
		break;
	case 0xD000:
	case 0xE000:
		index = ((A - 0xD000) >> 10) | (A & 0x03);
		vrc6.chr[index] = V;
		vrc6.cwrap(index << 10, V);
		break;
	case 0xF000:
		index = A & 0x03;
		switch (index) {
		case 0x00: VRCIRQ_Latch(V); break;
		case 0x01: VRCIRQ_Control(V); break;
		case 0x02: VRCIRQ_Acknowledge(); break;
		}
	}
}

void GenVRC6Power(void) {
	vrc6.prg[0] = 0;
	vrc6.prg[1] = 1;

	vrc6.chr[0] = 0;
	vrc6.chr[1] = 1;
	vrc6.chr[2] = 2;
	vrc6.chr[3] = 3;
	vrc6.chr[4] = 4;
	vrc6.chr[5] = 5;
	vrc6.chr[6] = 6;
	vrc6.chr[7] = 7;

	vrc6.mirr = 0;

	FixVRC6PRG();
	FixVRC6CHR();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC6Write);

	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void GenVRC6Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
	}
	WRAM = NULL;
}

void GenVRC6Restore(int version) {
	FixVRC6PRG();
	FixVRC6CHR();
}

void GenVRC6_Init(CartInfo *info, uint32 A0, uint32 A1, int wram) {
	vrc6.pwrap = GENPWRAP;
	vrc6.cwrap = GENCWRAP;
	vrc6.mwrap = GENMWRAP;

	vrc6_A0 = A0;
	vrc6_A1 = A1;

	if (wram) {
		if (info->iNES2) {
			WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
		} else if (info->mapper == 26) {
			WRAMSIZE = 8192;
		}
		if (WRAMSIZE) {
			WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
			SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
			AddExState(WRAM, WRAMSIZE, 0, "WRAM");
			if (info->battery) {
				info->SaveGame[0] = WRAM;
				info->SaveGameLen[0] = WRAMSIZE;
			}
		}
	}
	AddExState(&StateRegs, ~0, 0, 0);

	info->Power = GenVRC6Power;
	info->Close = GenVRC6Close;
	GameStateRestore = GenVRC6Restore;

	VRCIRQ_Init(TRUE);
	MapIRQHook = VRCIRQ_CPUHook;
	AddExState(&VRCIRQ_StateRegs, ~0, 0, 0);

	VRC6_ESI();
}

void NSFVRC6_Init(void) {
	VRC6_ESI();
	SetWriteHandler(0x8000, 0xbfff, VRC6SW);
}
