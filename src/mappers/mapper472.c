/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023-2024 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 472 denotes the 恒格 FK-206 JG MMC3-compatible PCB. It is basically mapper 52 with the bits reshuffled. */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg;
static uint8 dipsw;

static void M472CW(uint16 A, uint16 V) {
    uint16 mask = (reg & 0x20) ? 0x7F : 0xFF;
    uint16 base = reg << 3;

/*    FCEU_printf("CHR: A:%04x V:%02x R0:%02x\n", A, V, reg); */
	setchr1(A, (base & ~mask) | (V & mask));
}

static void M472PW(uint16 A, uint16 V) {
    uint16 mask = 0x0F;
    uint16 base = reg & 0xF0;

/*    FCEU_printf("PRG: A:%04x V:%02x R0:%02x\n", A, V, reg); */
	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(M472Write) {
/*    FCEU_printf("Wr: A:%04x V:%02x R0:%02x\n", A, V, reg); */
    if (MMC3_WramIsWritable()) {
	    reg = V;
	    MMC3_SyncPRG();
	    MMC3_SyncCHR();
    }
}

static DECLFR(M472Read) {
/*    FCEU_printf("Rd: A:%04x DIP:%02x\n", A, dipsw); */
	return dipsw;
}

static void M472Reset(void) {
    reg = 0;
	dipsw ^= 0x80; /* any other variants? */
	MMC3_Reset();
}

static void M472Power(void) {
    reg = 0;
	dipsw = 0x80; /* start with 4-in-1 menu */
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, M472Read);
	SetWriteHandler(0x6000, 0x7FFF, M472Write);
}

void Mapper472_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = M472CW;
	MMC3_pwrap = M472PW;
	info->Power = M472Power;
    info->Reset = M472Reset;
	AddExState(&reg, 1, 0, "EXPR");
}
