/* FCE Ultra - NES/Famicom Emulator
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
 *
 * VRC-2/VRC-4 Konami
 * VRC-4 Pirate
 *
 * VRC2
 * Nickname	PCB		A0	A1	Registers					iNES mapper	submapper
 * VRC2a	351618	A1	A0	$x000, $x002, $x001, $x003	22			0
 * VRC2b	many†	A0	A1	$x000, $x001, $x002, $x003	23			3
 * VRC2c	351948	A1	A0	$x000, $x002, $x001, $x003	25			3
 * VRC4
 * Nickname	PCB		A0	A1	Registers					iNES mapper	submapper
 * VRC4a	352398	A1	A2	$x000, $x002, $x004, $x006	21			1
 * VRC4b	351406	A1	A0	$x000, $x002, $x001, $x003	25			1
 * VRC4c	352889	A6	A7	$x000, $x040, $x080, $x0C0	21			2
 * VRC4d	352400	A3	A2	$x000, $x008, $x004, $x00C	25			2
 * VRC4e	352396	A2	A3	$x000, $x004, $x008, $x00C	23			2
 * VRC4f	-		A0	A1	$x000, $x001, $x002, $x003	23			1
 *
 */

#include "mapinc.h"
#include "vrc24.h"
#include "vrcirq.h"

static VRC24Type variant;
static uint8 autoConfig = 0;

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

VRC24 vrc24;

static SFORMAT StateRegs[] =
{
	{ vrc24.prgreg, 2, "PREG" },
	{ vrc24.chrreg, 8, "CREG" },
	{ vrc24.chrhi,  8, "CHRH" },
	{ &vrc24.cmd, 1, "CMDR" },
	{ &vrc24.mirr, 1, "MIRR" },
	{ &variant, 1, "VRNT" },

	{ 0 }
};

static uint32 translateAddr(uint32 A) {
	uint8 A0 = 0;
	uint8 A1 = 0;

	if (autoConfig) {
		switch (variant) {
		/* 21 */
		case VRC4a:
		case VRC4c:
			A0 = (A >> 1) & 0x01;
			A1 = (A >> 2) & 0x01;

			A0 |= (A >> 6) & 0x01;
			A1 |= (A >> 7) & 0x01;
			break;

		/* 23 */
		case VRC2b:
		case VRC4f:
		case VRC4e:
			A0 = (A >> 0) & 0x01;
			A1 = (A >> 1) & 0x01;

			A0 |= (A >> 2) & 0x01;
			A1 |= (A >> 3) & 0x01;
			break;

		/* 25 */
		case VRC2c:
		case VRC4b:
		case VRC4d:
			A0 = (A >> 1) & 0x01;
			A1 = (A >> 0) & 0x01;

			A0 |= (A >> 3) & 0x01;
			A1 |= (A >> 2) & 0x01;
			break;

		default:
			break;
		}
	} else {
		switch (variant) {
		/* 21 */
		case VRC4a:
			A0 = (A >> 1) & 0x01;
			A1 = (A >> 2) & 0x01;
			break;

		case VRC4c:
			A0 = (A >> 6) & 0x01;
			A1 = (A >> 7) & 0x01;
			break;

		/* 22 */
		case VRC2a:
			A0 = (A >> 1) & 0x01;
			A1 = (A >> 0) & 0x01;
			break;

		/* 23 */
		case VRC2b:
		case VRC4f:
			A0 = (A >> 0) & 0x01;
			A1 = (A >> 1) & 0x01;
			break;

		case VRC4e:
			A0 |= (A >> 2) & 0x01;
			A1 |= (A >> 3) & 0x01;
			break;

		/* 25 */
		case VRC2c:
		case VRC4b:
			A0 = (A >> 1) & 0x01;
			A1 = (A >> 0) & 0x01;
			break;

		case VRC4d:
			A0 |= (A >> 3) & 0x01;
			A1 |= (A >> 2) & 0x01;
			break;

		case VRC4_544:
		case VRC4_559:
			A0 |= (A >> 10) & 0x01;
			A1 |= (A >> 11) & 0x01;
			break;

		default:
			break;
		}
	}

	return (A & 0xF000) | (A1 << 1) | (A0 << 0);
}

void FixVRC24PRG(void) {
	if (vrc24.cmd & 2) {
		vrc24.pwrap(0x8000, ~1);
		vrc24.pwrap(0xC000, vrc24.prgreg[0]);
	} else {
		vrc24.pwrap(0x8000, vrc24.prgreg[0]);
		vrc24.pwrap(0xC000, ~1);
	}
	vrc24.pwrap(0xA000, vrc24.prgreg[1]);
	vrc24.pwrap(0xE000, ~0);
}

void FixVRC24CHR(void) {
	int i;

	for (i = 0; i < 8; i++) {
		vrc24.cwrap(0x0400 * i, (vrc24.chrhi[i] << 4) | vrc24.chrreg[i]);
	}
	if (vrc24.mwrap) {
		vrc24.mwrap(vrc24.mirr);
	}
}

DECLFW(VRC24Write) {
	uint8 index;

	A = translateAddr(A) & 0xF003;

	switch (A & 0xF000) {
	case 0x8000:
	case 0xA000:
		vrc24.prgreg[(A >> 13) & 0x01] = V & 0x1F;
		FixVRC24PRG();
		break;

	case 0x9000:
		switch (A & 0x03) {
		case 0:
		case 1:
			if (V != 0xFF) {
				if (vrc24.mwrap) {
					vrc24.mwrap(V);
				}
			}
			break;
		case 2:
		case 3:
			vrc24.cmd = V;
			FixVRC24PRG();
			break;
		}
		break;

	case 0xF000:
		switch (A & 0x03) {
		case 0x00: VRCIRQ_LatchNibble(V, 0); break;
		case 0x01: VRCIRQ_LatchNibble(V, 1); break;
		case 0x02: VRCIRQ_Control(V); break;
		case 0x03: VRCIRQ_Acknowledge(); break;
		}
		break;

	default:
		index = ((A >> 1) & 0x01) | ((A - 0xB000) >> 11);
		if (A & 0x01) {
			/* m25 can be 512K, rest are 256K or less */
			vrc24.chrhi[index] = V & 0x1F;
		} else {
			vrc24.chrreg[index] = V & 0xF;
		}
		vrc24.cwrap(index << 10, ((vrc24.chrhi[index] << 4) | vrc24.chrreg[index]));
		break;
	}
}

static void GENPWRAP(uint32 A, uint8 V) {
	setprg8(A, V & 0x1F);
}

static void GENCWRAP(uint32 A, uint32 V) {
	setchr1(A, V & 0x1FF);
}

static void GENMWRAP(uint8 V) {
	vrc24.mirr = V;

	switch (V & 0x3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

void GenVRC24Power(void) {
	vrc24.prgreg[0] = 0;
	vrc24.prgreg[1] = 1;

	vrc24.chrreg[0] = 0;
	vrc24.chrreg[1] = 1;
	vrc24.chrreg[2] = 2;
	vrc24.chrreg[3] = 3;
	vrc24.chrreg[4] = 4;
	vrc24.chrreg[5] = 5;
	vrc24.chrreg[6] = 6;
	vrc24.chrreg[7] = 7;

	vrc24.chrhi[0] = vrc24.chrhi[1] = vrc24.chrhi[2] = vrc24.chrhi[3] = 0;
	vrc24.chrhi[4] = vrc24.chrhi[5] = vrc24.chrhi[6] = vrc24.chrhi[7] = 0;

	vrc24.cmd = vrc24.mirr = 0;

	FixVRC24PRG();
	FixVRC24CHR();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC24Write);

	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}

	if (UNIFchrrama) {
		setchr8(0);
	}
}

void GenVRC24Restore(int version) {
	FixVRC24PRG();
	FixVRC24CHR();
}

void GenVRC24Close(void) {
	if (WRAM) {
		FCEU_gfree(WRAM);
	}
	WRAM = NULL;
}

void GenVRC24_Init(CartInfo *info, VRC24Type type, int wram) {
	vrc24.pwrap = GENPWRAP;
	vrc24.cwrap = GENCWRAP;
	vrc24.mwrap = GENMWRAP;

	WRAMSIZE = wram ? 8192 : 0;
	variant = type;

	if (wram) {
		WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");

		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}

	AddExState(&StateRegs, ~0, 0, 0);

	info->Power = GenVRC24Power;
	info->Close = GenVRC24Close;
	GameStateRestore = GenVRC24Restore;

	VRCIRQ_Init();
}

/* -------------------- Mapper 21 -------------------- */

void Mapper21_Init(CartInfo *info) {
	/* Mapper 21 - VRC4a, VRC4c */
	VRC24Type type = VRC4a;
	autoConfig = 1;
	if (info->iNES2 && !info->submapper) {
		switch (info->submapper) {
		case 1: type = VRC4a; autoConfig = 0; break;
		case 2: type = VRC4c; autoConfig = 0; break;
		}
	}
	GenVRC24_Init(info, type, 1);
}

/* -------------------- Mapper 22 -------------------- */

static void M22CW(uint32 A, uint32 V) {
	setchr1(A, V >> 1);
}

void Mapper22_Init(CartInfo *info) {
	/* Mapper 22 - VRC2a */
	autoConfig = 0;
	GenVRC24_Init(info, VRC2a, 0);
	vrc24.cwrap = M22CW;
}

/* -------------------- Mapper 23 -------------------- */

void Mapper23_Init(CartInfo *info) {
	/* Mapper 23 - VRC2b, VRC4e, VRC4f */
	VRC24Type type = VRC4f;
	autoConfig = 1;
	if (info->iNES2 && !info->submapper) {
		switch (info->submapper) {
		case 1: type = VRC4f; autoConfig = 0; break;
		case 2: type = VRC4e; autoConfig = 0; break;
		case 3: type = VRC2b; autoConfig = 0; break;
		}
	}
	GenVRC24_Init(info, type, 1);
}

/* -------------------- Mapper 25 -------------------- */

void Mapper25_Init(CartInfo *info) {
	/* Mapper 25 - VRC2c, VRC4b, VRC4d */
	VRC24Type type = VRC4b;
	autoConfig = 1;
	if (info->iNES2 && !info->submapper) {
		switch (info->submapper) {
		case 1: type = VRC4b; autoConfig = 0; break;
		case 2: type = VRC4d; autoConfig = 0; break;
		case 3: type = VRC2c; autoConfig = 0; break;
		}
	}
	GenVRC24_Init(info, type, 1);
}
