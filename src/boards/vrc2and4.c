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
#include "vrc2and4.h"
#include "vrcirq.h"

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static uint8 vrc2and4_VRC4;
static uint32 vrc2and4_A0;
static uint32 vrc2and4_A1;

VRC24 vrc24;

static SFORMAT StateRegs[] =
{
	{ vrc24.prgreg, 2, "PREG" },
	{ vrc24.chrreg, 16, "CREG" },
	{ &vrc24.cmd, 1, "CMDR" },
	{ &vrc24.mirr, 1, "MIRR" },

	{ 0 }
};

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
		vrc24.cwrap(0x0400 * i, vrc24.chrreg[i]);
	}
	if (vrc24.mwrap) {
		vrc24.mwrap(vrc24.mirr);
	}
}

DECLFW(VRC24Write) {
	uint8 index;

	switch (A & 0xF000) {
	case 0x8000:
	case 0xA000:
		vrc24.prgreg[(A >> 13) & 0x01] = V & 0x1F;
		FixVRC24PRG();
		break;

	case 0x9000:
		index = ((A & vrc2and4_A1) ? 0x02 : 0x00) | ((A & vrc2and4_A0) ? 0x01 : 0x00);
		switch (index & (vrc2and4_VRC4 ? 0x03 : 0x00)) {
		case 0:
		case 1:
			if (V != 0xFF) {
				if (vrc24.mwrap) {
					vrc24.mwrap(V);
				}
			}
			break;
		case 2:
			vrc24.cmd = V;
			FixVRC24PRG();
			break;
		case 3:
			if (vrc24.writeMisc) {
				vrc24.writeMisc(A, V);
			}
			break;
		}
		break;

	case 0xF000:
		index = ((A & vrc2and4_A1) ? 0x02 : 0x00) | ((A & vrc2and4_A0) ? 0x01 : 0x00);
		switch (index) {
		case 0x00: VRCIRQ_LatchNibble(V, 0); break;
		case 0x01: VRCIRQ_LatchNibble(V, 1); break;
		case 0x02: VRCIRQ_Control(V); break;
		case 0x03: VRCIRQ_Acknowledge(); break;
		}
		break;

	default:
		index = (((A & 0xF000) - 0xB000) >> 11) | ((A & vrc2and4_A1) ? 0x01 : 0x00);
		if (A & vrc2and4_A0) {
			/* m25 can be 512K, rest are 256K or less */
			vrc24.chrreg[index] = (vrc24.chrreg[index] & 0x000F) | (V << 4);
		} else {
			vrc24.chrreg[index] = (vrc24.chrreg[index] & 0x0FF0) | (V & 0x0F);
		}
		vrc24.cwrap(index << 10, vrc24.chrreg[index]);
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

	switch (V & (vrc2and4_VRC4 ? 0x03 : 0x01)) {
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

void GenVRC24_Init(CartInfo *info, uint8 vrc4, uint32 A0, uint32 A1, int wram, int irqRepeated) {
	vrc24.pwrap = GENPWRAP;
	vrc24.cwrap = GENCWRAP;
	vrc24.mwrap = GENMWRAP;
	vrc24.writeMisc = NULL;

	vrc2and4_A0 = A0;
	vrc2and4_A1 = A1;
	vrc2and4_VRC4 = vrc4;

	WRAMSIZE = wram ? 8192 : 0;

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

	VRCIRQ_Init(irqRepeated);
}
