/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* NES 2.0 Mapper 398 - PCB YY840820C
 * 1995 Super HiK 5-in-1 - 新系列米奇老鼠組合卡 (JY-048)
 */

#include "mapinc.h"
#include "vrc2and4.h"

static uint8 latch;
static uint8 PPUCHRBus;

static SFORMAT StateRegs[] = {
	{ &latch, 1, "LATC" },
	{ &PPUCHRBus, 1, "PPUC" },
	{ 0 },
};

static void M398PW(uint32 A, uint8 V) {
	if (latch & 0x80) {
        /* GNROM-like */
		setprg32(0x8000, ((latch >> 5) & 0x06) | ((vrc24.chrreg[PPUCHRBus] >> 2) & 0x01));
	} else {
		setprg8(A, V & 0x0F);
	}
}

static void M398CW(uint32 A, uint32 V) {
	if (latch & 0x80) {
        /* GNROM-like */
		setchr8(0x40 | ((latch >> 3) & 0x08) | (vrc24.chrreg[PPUCHRBus] & 0x07));
	} else {
		setchr1(A, V & 0x1FF);
	}
}

static DECLFW(M398WriteLatch) {
	latch = A & 0xFF;
	FixVRC24PRG();
	FixVRC24CHR();
	VRC24Write(A, V);
}

static void FP_FASTAPASS(1) M398PPUHook(uint32 A) {
	uint8 bank = (A & 0x1FFF) >> 10;
	if ((PPUCHRBus != bank) && ((A & 0x3000) != 0x2000)) {
		PPUCHRBus = bank;
		FixVRC24PRG();
		FixVRC24CHR();
	}
}

static void M398Reset(void) {
	latch = 0xC0;
	FixVRC24PRG();
	FixVRC24CHR();
}

static void M398Power(void) {
	PPUCHRBus = 0;
    latch = 0xC0;
	GenVRC24Power();
	SetWriteHandler(0x8000, 0xFFFF, M398WriteLatch);
}

void Mapper398_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC4, 0x01, 0x02, 0, 1);
	info->Reset = M398Reset;
	info->Power = M398Power;
	PPU_hook = M398PPUHook;
	vrc24.pwrap = M398PW;
	vrc24.cwrap = M398CW;
	AddExState(StateRegs, ~0, 0, 0);
}
