/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * FDS Conversion
 * - Submapper 1 - Ai Senshi Nicol (256K PRG, 128K CHR, fixed Mirroring)
 * - Submapper 3 - Bio Miracle Bokutte Upa (J) (128K PRG, 0K CHR, IRQ)
 * 
 * - Submapper 2
 * - KS-018/AC-08/LH09
 * - UNIF UNL-AC08
 * - [UNIF] Green Beret (FDS Conversion, LH09) (Unl) [U][!][t1] (160K PRG)
 * - Green Beret (FDS Conversion) (Unl) (256K PRG)
 */

#include "mapinc.h"
#include "../fds_apu.h"

static uint8 reg, prg, chr, mirr;
static uint8 IRQa;
static uint16 IRQCount;

static SFORMAT StateRegs[] =
{
	{ &reg, 1, "REG0" },
	{ &prg, 1, "PREG" },
	{ &chr, 1, "CREG" },
	{ &mirr, 1, "MIRR" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQCount, 2, "IRQC" },
	
	{ 0 }
};

/* Submapper 1 - Ai Senshi Nicol */

static void M042_Sub1_Sync(void) {
	setprg8(0x6000, prg & 0x0F);
	setprg32(0x8000, ~0);
	setchr8(chr & 0x0F);
}

static DECLFW(M042_Sub1_Write) {
	switch (A & 0xE000) {
	case 0x8000:
		chr = V;
		M042_Sub1_Sync();
		break;
	case 0xE000:
		prg = V;
		M042_Sub1_Sync();
		break;
	}
}

static void M042_Sub1_Restore(int version) {
	M042_Sub1_Sync();
}

static void M042_Sub1_Power(void) {
	prg = 0;
	chr = 0;
	FDSSoundPower();
	M042_Sub1_Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, M042_Sub1_Write);
}

/* Submapper 2 - Green Beret */

static void M042_Sub2_Sync(void) {
	uint8 prg = (ROM.prg.size & 0x0F) ? 4 : 7;
	setprg8(0x6000, (reg >> 1) & 0x0F);
	setprg32(0x8000, prg);
	setchr8(0);
	setmirror(((mirr >> 3) & 1) ^ 1);
}

static DECLFW(M042_Sub2_Write) {
	switch (A & 0xF001) {
	case 0x4001:
	case 0x4000:
		if ((A & 0xFF) != 0x25) {
			break;
		}
		mirr = V;
		M042_Sub2_Sync();
		break;
	case 0x8001:
		reg = V;
		M042_Sub2_Sync();
		break;
	}
}

static void M042_Sub2_Restore(int version) {
	M042_Sub2_Sync();
}

static void M042_Sub2_Power(void) {
	reg = 0;
	mirr = 0;
	FDSSoundPower();
	M042_Sub2_Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0xFFFF, M042_Sub2_Write);
}

/* Submapper 3 - Mario Baby */

static void M042_Sub3_Sync(void) {
	setprg8(0x6000, prg & 0x0F);
	setprg32(0x8000, ~0);
	setchr8(0);
	setmirror(((mirr >> 3) & 1) ^ 1);
}

static DECLFW(M042_Sub3_Write) {
	switch (A & 0xE003) {
	case 0xE000:
		prg = V;
		M042_Sub3_Sync();
		break;
	case 0xE001:
		mirr = V;
		M042_Sub3_Sync();
		break;
	case 0xE002:
		IRQa = V;
		break;
	}
}

static void M042_Sub3_Restore(int version) {
	M042_Sub3_Sync();
}

static void M042_Sub3_Power(void) {
	prg = 0;
	mirr = 0;
	M042_Sub3_Sync();
	FDSSoundPower();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xE000, 0xFFFF, M042_Sub3_Write);
}

static void FP_FASTAPASS(1) M042_Sub3_IRQHook(int a) {
	while (a--) { /* NOTE: Possible performance hit, but whatever */
		if (IRQa & 0x02) {
			IRQCount++;
			if ((IRQCount & 0x6000) == 0x6000) {
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				X6502_IRQEnd(FCEU_IQEXT);
			}
		} else {
			IRQCount = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
	}
}

/* Mapper 42 Loader */

void Mapper042_Init(CartInfo *info) {
	if (info->submapper == 0 || info-> submapper > 3) {
		if (!UNIFchrrama) {
			/* Ai Senshi Nicole, only cart with CHR-ROM, all others use CHR-RAM */
			info->submapper = 1;
		} else {
			if ((ROM.prg.size * 16) > 128) {
				/* Green Beret LH09 FDS Conversion can be 160K or 256K */
				info->submapper = 2;
			} else {
				/* Mario Baby has only 128K PRG */
				info->submapper = 3;
			}
		}
	}
	switch (info->submapper) {
	case 1:
		info->Power = M042_Sub1_Power;
		GameStateRestore = M042_Sub1_Restore;
		break;
	case 2:
		info->Power = M042_Sub2_Power;
		GameStateRestore = M042_Sub2_Restore;
		break;
	default:
		info->Power = M042_Sub3_Power;
		MapIRQHook = M042_Sub3_IRQHook;
		GameStateRestore = M042_Sub3_Restore;
		break;
	}
	AddExState(StateRegs, ~0, 0, NULL);
}
