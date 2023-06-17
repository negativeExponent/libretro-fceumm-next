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
 */

/* iNES Mapper 076
 * iNES Mapper 076 represents NAMCOT-3446, a board used onlyfor the game
 * Megami Tensei: Digital Devil Story.
 * It rewires the Namcot 108 mapper IC to be able to address 128 KiB of CHR,
 * in exchange for coarser CHR banking.
 * The PCB also has a 7432 which allows the use of a 28-pin CHR-ROM.
 * https://www.nesdev.org/wiki/INES_Mapper_076
 */

#include "mapinc.h"

static uint8 reg[8];
static uint8 cmd;

static SFORMAT StateRegs[] = {
    { reg, 8, "REGS" },
    { &cmd, 1, "CMD\0" },
    { 0 }
};

static void Sync(void) {
    setprg8(0x8000, reg[6] & 0x1F);
    setprg8(0xA000, reg[7] & 0x1F);
    setprg8(0xC000,   (~1) & 0x1F);
    setprg8(0xE000,   (~0) & 0x1F);

    setchr2(0x0000, reg[2] & 0x3F);
    setchr2(0x0800, reg[3] & 0x3F);
    setchr2(0x1000, reg[4] & 0x3F);
    setchr2(0x1800, reg[5] & 0x3F);
}

static DECLFW(M076Write) {
    A &= 0xE001;
    switch (A) {
    case 0x8000:
        cmd = V;
        break;
    case 0x8001:
        reg[cmd & 0x07] = V;
        break;
    }
    Sync();
}

static void StateRestore(int version) {
    Sync();
}

static void M076Power(void) {
    cmd = 0;

    reg[0] = 0;
    reg[1] = 2;
    reg[2] = 4;
    reg[3] = 5;
    reg[4] = 6;
    reg[5] = 7;

    reg[6] = 0;
    reg[7] = 1;

    Sync();
    
    SetReadHandler(0x8000, 0xFFFF, CartBR);
    SetWriteHandler(0x8000, 0x9FFF, M076Write);
}

void Mapper076_Init(CartInfo *info) {
	info->Power = M076Power;
    GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
