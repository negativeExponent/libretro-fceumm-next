/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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

/* NES 2.0 Mapper 470 denotes the INX_007T_V01 multicart circuit board, 
 * used for the Retro-Bit re-release of Battletoads and Double Dragon.
 * It is basically AOROM with an additional outer bank register at $5000-$5FFF
 * whose data selects the 256 KiB outer bank.
 */

#include "mapinc.h"
#include "latch.h"

static uint8 reg;

static SFORMAT StateRegs[] = {
    { &reg, 1, "REGS" },
    { 0 }
};

static void Sync(void) {
	setprg32(0x8000, (reg << 3) | (latch.data & 0x07));
	setchr8(0);
    setmirror((MI_0 + (latch.data >> 4) & 1));
}

static DECLFW(M470Write5) {
	reg = V;
    Sync();
}

static void M470Reset(void) {
    reg = 0;
    Latch_RegReset();
}

static void M470Power(void) {
    reg = 0;
	Latch_Power();
    SetWriteHandler(0x5000, 0x5FFF, M470Write5);
}

void Mapper470_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = M470Power;
    info->Reset = M470Reset;
    AddExState(StateRegs, ~0, 0, NULL);
}
