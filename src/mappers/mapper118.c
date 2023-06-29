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

#include "mapinc.h"
#include "mmc3.h"

static uint8 NTPage[4] = { 0, 1, 0, 1 };

static void GENNOMWRAP(uint8 V) {
    /* do nothing */
}

static void M118CW(uint32 A, uint8 V) {
    setchr1(A, V & 0x7F);

    if (A < 0x1000) {
        setntamem(NTARAM + ((V >> 7) << 10), 1, A >> 10);
    }
}

void Mapper118_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_mwrap = GENNOMWRAP;
    MMC3_cwrap = M118CW;
	AddExState(NTPage, 4, 0, "NTAR");
}
