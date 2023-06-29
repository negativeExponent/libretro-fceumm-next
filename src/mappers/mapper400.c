/* FCEUmm - NES/Famicom Emulator
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
 *
 * NES 2.0 Mapper 400 is used for retroUSB's 8-BIT XMAS 2017 homebrew cartridge.
 *
 */

#include "mapinc.h"
#include "latch.h"

static uint8 reg;
static uint8 led;

static SFORMAT StateRegs[] = {
	{ &reg, 1, "REG0" },

	{ 0 }
};

static void Sync(void) {
    uint8 outer = reg & ~7;
	setprg16(0x8000, outer | (latch.data & 7));
	setprg16(0xC000, outer | 7);
	setchr8(0);
    if (outer == 0x80) {
        /* use default mirroring */
    } else {
        setmirror(((outer >> 5) & 1) ^ 1);
    }
}

static DECLFW(M400WriteReg) {
    if (A & 0x800) {
        reg = V;
        Sync();
    }
}

static DECLFW(M400WriteLED) {
    led = V;
}

static void M400Reset(void) {
	reg = 0x80;
	LatchHardReset();
}

static void M400Power() {
    reg = 0x80;
    LatchPower();
    SetWriteHandler(0x7000, 0x7FFF, M400WriteReg);
    SetWriteHandler(0x8000, 0xBFFF, M400WriteLED);
}

void Mapper400_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 1);
	info->Power = M400Power;
    info->Reset = M400Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
