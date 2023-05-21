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

/* NES 2.0 Mapper 362 - PCB 830506C
 * 1995 Super HiK 4-in-1 (JY-005)
 */

#include "mapinc.h"
#include "vrc24.h"

static uint8 game;
static uint8 PPUCHRBus;

static SFORMAT StateRegs[] = {
    { &game,          1, "GAME" },
    { &PPUCHRBus,     1, "PPUC" },
	{ 0 },
};

static void M362PW(uint32 A, uint8 V) {
	uint8 prgBase = (game == 0) ? ((vrc24.chrhi[PPUCHRBus] << 1) & 0x30) : 0x40;
	setprg8(A, prgBase | (V & 0x0F));
}

static void M362CW(uint32 A, uint32 V) {
	uint32 chrBase = (game == 0) ? ((vrc24.chrhi[PPUCHRBus] << 4) & 0x180) : 0x200;
	uint32 chrMask = (game == 0) ? 0x7F : 0x1FF;
	setchr1(A, chrBase | (V & chrMask));
}

static DECLFW(M362CHRWrite) {
	VRC24Write(A, V);
	if (A & 0x01) {
		/* NOTE: Because the lst higher 2 CHR-ROM bits are repurposed as PRG/CHR outer bank, 
	 	an extra PRG sync after a CHR write. */
		FixVRC24PRG();
	}
}

static DECLFW(M362IRQWrite) {
	/* NOTE: acknowledge IRQ but do not move A control bit to E control bit.
	 * So, just make E bit the same with A bit coz im lazy to modify IRQ code for now. */
	if ((A & 0x03) == 0x02) {
		V |= ((V >> 1) & 0x01);
	}
	VRC24Write(A, V);
}

static void FP_FASTAPASS(1) M362PPUHook(uint32 A) {
    uint8 bank = (A & 0x1FFF) >> 10;
	if ((game == 0) && (PPUCHRBus != bank) && ((A & 0x3000) != 0x2000)) {
		PPUCHRBus = bank;
		FixVRC24CHR();
		FixVRC24PRG();
	}
}

static void M362Reset(void) {
	if (PRGsize[0] <= (512 * 1024)) {
        game = 0;
    } else {
        game = (game + 1) & 0x01;
    }
	FixVRC24CHR();
	FixVRC24PRG();
}

static void M362Power(void) {	
	PPUCHRBus = game = 0;
    GenVRC24Power();
	SetWriteHandler(0xB000, 0xEFFF, M362CHRWrite);
	SetWriteHandler(0xF000, 0xFFFF, M362IRQWrite);
}

void Mapper362_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC4f, 0);
	info->Reset = M362Reset;
	info->Power = M362Power;
    PPU_hook = M362PPUHook;
	vrc24.pwrap = M362PW;
	vrc24.cwrap = M362CW;
	AddExState(StateRegs, ~0, 0, 0);
}
