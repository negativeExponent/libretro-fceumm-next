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

/* NES 2.0 Mapper 447 - KL-06 (VRC4 clone)
 * 1993 New 860-in-1 Over-Valued Golden Version Games multicart
 */

#include "mapinc.h"
#include "vrc2and4.h"

static uint8 reg;

static SFORMAT StateRegs[] = {
	{ &reg, 1, "REGS" },
	{ 0 },
};

static void M447PW(uint32 A, uint8 V) {
    uint16 mask = 0x0F;
    uint16 base = reg << 4;

    if (reg & 0x04) {
        if (reg & 2) { /* NROM-128 */
            setprg8(0x8000, (base & ~mask) | (vrc24.prg[0] & mask));
            setprg8(0xA000, (base & ~mask) | (vrc24.prg[1] & mask));
            setprg8(0xC000, (base & ~mask) | (vrc24.prg[0] & mask));
            setprg8(0xE000, (base & ~mask) | (vrc24.prg[1] & mask));
        } else { /* NROM-256 */
            setprg8(0x8000, (base & ~mask) | ((vrc24.prg[0] & ~0x02) & mask));
            setprg8(0xA000, (base & ~mask) | ((vrc24.prg[1] & ~0x02) & mask));
            setprg8(0xC000, (base & ~mask) | ((vrc24.prg[0] |  0x02) & mask));
            setprg8(0xE000, (base & ~mask) | ((vrc24.prg[1] |  0x02) & mask));
        }
    } else {
        setprg8(A, (base & ~mask) | (V & mask));
    }
}

static void M447CW(uint32 A, uint32 V) {
    setchr1(A, (reg << 7) | (V & 0x7F));
}

static DECLFW(M447WriteReg) {
    CartBW(A, V);
	if ((vrc24.cmd & 0x01) && !(reg & 0x01)) {
		reg = A & 0xFF;
		VRC24_FixPRG();
        VRC24_FixCHR();
	}
}

static void M447Reset(void) {
	reg = 0;
	VRC24_FixPRG();
    VRC24_FixCHR();
}

static void M447Power(void) {
	reg = 0;
	VRC24_Power();
	SetWriteHandler(0x6000, 0x7FFF, M447WriteReg);
}

void Mapper447_Init(CartInfo *info) {
    VRC24_Init(info, VRC4, 0x04, 0x08, 1, 1);
	info->Reset = M447Reset;
	info->Power = M447Power;
    VRC24_pwrap = M447PW;
    VRC24_cwrap = M447CW;
	AddExState(StateRegs, ~0, 0, NULL);
}
