/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007-2008 Mad Dumper, CaH4e3
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
 * Panda prince pirate.
 * MK4, MK6, A9711/A9713 board
 * 6035052 seems to be the same too, but with prot array in reverse
 * A9746  seems to be the same too, check
 * 187 seems to be the same too, check (A98402 board)
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg[5];
static uint8 prot_lut;
static uint8 prot_idx;
static uint8 prot_value;
static uint8 banks_updated;

static SFORMAT StateRegs[] = {
	{ reg, 5, "EXPR" },
	{ &prot_lut, 1, "PRLT" },
	{ &prot_idx, 1, "PRID" },
	{ &prot_value, 1, "PRVL" },
	{ &banks_updated, 1, "BANK" },
	{ 0 }
};

static void Sync() {
	switch (prot_idx & 0x3F) {
	case 0x20:
		banks_updated = 1;
		reg[0] = prot_value;
		break;
	case 0x29:
		banks_updated = 1;
		reg[0] = prot_value;
		break;
	case 0x26:
		banks_updated = 0;
		reg[0] = prot_value;
		break;
	case 0x2B:
		banks_updated = 1;
		reg[0] = prot_value;
		break;
	case 0x2C:
		banks_updated = 1;
		if (prot_value) {
			reg[0] = prot_value;
		}
		break;
	case 0x3C:
	case 0x3F:
		banks_updated = 1;
		reg[0] = prot_value;
		break;
	case 0x28:
		banks_updated = 0;
		reg[1] = prot_value;
		break;
	case 0x2A:
		banks_updated = 0;
		reg[2] = prot_value;
		break;
	case 0x2F:
		break;
	default:
		prot_idx = 0;
		break;
	}
}

static void M121CW(uint32 A, uint8 V) {
	if ((ROM.prg.size * 16) > 256) { /* A9713 multigame extension hack! */
		setchr1(A, V | ((reg[4] & 0x80) << 1));
	} else {
		if ((A & 0x1000) && (mmc3.cmd & 0x80)) {
			setchr1(A, V | 0x100);
		} else {
			setchr1(A, V);
		}
	}
}

static void M121PW(uint32 A, uint8 V) {
	setprg8(A, (V & 0x1F) | ((reg[4] & 0x80) >> 2));
	if (prot_idx & 0x3F) {
		setprg8(0xE000, (reg[0]) | ((reg[4] & 0x80) >> 2));
		setprg8(0xC000, (reg[1]) | ((reg[4] & 0x80) >> 2));
		setprg8(0xA000, (reg[2]) | ((reg[4] & 0x80) >> 2));
	}
}

static DECLFW(M121Write) {
	/*	FCEU_printf("write: %04x:%04x\n",A&0xE003,V); */
	switch (A & 0xE003) {
	case 0x8000:
		/*		FCEU_printf("gen: %02x\n",V); */
		MMC3_CMDWrite(A, V);
		MMC3_FixPRG();
		break;
	case 0x8001:
		prot_value =
			((V << 5) & 0x20) |
			((V << 3) & 0x10) |
			((V << 1) & 0x08) |
			((V >> 1) & 0x04) |
			((V >> 3) & 0x02) |
			((V >> 5) & 0x01);
		/* FCEU_printf("bank: %02x (%02x)\n",V,prot_value); */
		if (!banks_updated) {
			Sync();
		}
		MMC3_CMDWrite(A, V);
		MMC3_FixPRG();
		break;
	case 0x8003:
		prot_idx = V & 0x3F;
		/* banks_updated = 0; */
		/* FCEU_printf("prot: %02x\n",prot_idx); */
		Sync();
		MMC3_CMDWrite(0x8000, V);
		MMC3_FixPRG();
		break;
	}
}

static uint8 prot_array[16] = { 0x83, 0x83, 0x42, 0x00, 0x00, 0x02, 0x02, 0x03 };
static DECLFW(M121WriteLUT) {
	prot_lut = prot_array[((A >> 6) & 0x04) | (V & 0x03)]; /* 0x100 bit in address seems to be switch arrays 0, 2, 2, 3 (Contra Fighter) */
	if (A & 0x100) { /* A9713 multigame extension */
		reg[4] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
	/*	FCEU_printf("write: %04x:%04x\n",A,V); */
}

static DECLFR(M121ReadLUT) {
	/*	FCEU_printf("read:  %04x->\n",A,reg[0]); */
	return prot_lut;
}

static void M121Power(void) {
	reg[4] = 0;
	prot_lut = 0;
	prot_idx = 0;
	prot_value = 0;
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, M121ReadLUT);
	SetWriteHandler(0x5000, 0x5FFF, M121WriteLUT);
	SetWriteHandler(0x8000, 0x9FFF, M121Write);
}

void Mapper121_Init(CartInfo *info) {
	MMC3_Init(info, 0, 0);
	MMC3_pwrap = M121PW;
	MMC3_cwrap = M121CW;
	info->Power = M121Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
