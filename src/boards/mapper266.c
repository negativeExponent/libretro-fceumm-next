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
 * City Fighter IV sith Sound VRC4 hacked
 */

#include "mapinc.h"
#include "vrc2and4.h"

static uint8 prg_reg;
static writefunc pcmwrite;

static SFORMAT StateRegs[] =
{
	{ &prg_reg, 1, "PREG" },
	{ 0 }
};

static void M266PW(uint32 A, uint8 V) {
	setprg32(0x8000, prg_reg >> 2);
}

static DECLFW(M266Write) {
	/* FCEU_printf("%04x %02x",A,V); */
	switch (A & 0xF00C) {
	case 0x900C:
		if (A & 0x800) {
			pcmwrite(0x4011, (V & 0xf) << 3);
		} else {
			prg_reg = V & 0xC;
			FixVRC24PRG();
		}
		break;
	default:
		A = (A & ~0x6000) | ((A << 1) & 0x4000) | ((A >> 1) & 0x2000);
		VRC24Write(A, V);
		break;
	}
}

static void M266Power(void) {
	prg_reg = 0;
	GenVRC24Power();
	pcmwrite = GetWriteHandler(0x4011);
	SetWriteHandler(0x8000, 0xFFFF, M266Write);
}

void Mapper266_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC4, 0x04, 0x08, 0, 1);
	info->Power = M266Power;
	vrc24.pwrap = M266PW;
	AddExState(&StateRegs, ~0, 0, 0);
}
