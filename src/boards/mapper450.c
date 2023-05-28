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

/* NES 2.0 Mapper 450 - YY841157C
 */

#include "mapinc.h"
#include "vrc2and4.h"

/* TODO: move this register to VRC2 microwire interface when implemented */
static uint8 wires;

static SFORMAT StateRegs[] = {
	{ &wires, 1, "WIRE" },
	{ 0 },
};

static void M450PW(uint32 A, uint8 V) {
    setprg8(A, (wires << 4) | (V & 0x0F));
}

static void M450CW(uint32 A, uint32 V) {
    setchr1(A, (wires << 7) | (V & 0x7F));
}

static DECLFW(M450WriteReg) {
	wires = V;
	FixVRC24PRG();
    FixVRC24CHR();
}

static void M450Power(void) {
	wires = 0;
	GenVRC24Power();
	SetWriteHandler(0x6000, 0x6FFF, M450WriteReg);
}

void Mapper450_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC2, 0x01, 0x02, 0, 1);
	info->Power = M450Power;
    vrc24.pwrap = M450PW;
    vrc24.cwrap = M450CW;
	AddExState(StateRegs, ~0, 0, 0);
}
