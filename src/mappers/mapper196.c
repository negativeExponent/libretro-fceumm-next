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

/* ---------------------------- Mapper 196 ------------------------------- */
/* MMC3 board with optional command address line connection, allows to
 * make three-four different wirings to IRQ address lines and separately to
 * CMD address line, Mali Boss additionally check if wiring are correct for
 * game
 */

static void M196PRG(void) {
	if (mmc3.expregs[0]) {
		setprg32(0x8000, mmc3.expregs[1]);
    } else {
		setprg8(0x8000, mmc3.regs[6]);
        setprg8(0xA000, mmc3.regs[7]);
        setprg8(0xC000, ~1);
        setprg8(0xE000, ~0);
    }
}

static DECLFW(M196Write) {
	A = (A & 0xF000) | (!!(A & 0xE) ^ (A & 0x01));
	MMC3_Write(A, V);
}

static DECLFW(M196WriteLo) {
	mmc3.expregs[0] = 1;
	mmc3.expregs[1] = (V & 0xf) | (V >> 4);
	MMC3_FixPRG();
}

static void M196Power(void) {
	GenMMC3Power();
	mmc3.expregs[0] = mmc3.expregs[1] = 0;
	SetWriteHandler(0x6000, 0x7FFF, M196WriteLo);
	SetWriteHandler(0x8000, 0xFFFF, M196Write);
}

void Mapper196_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	MMC3_FixPRG = M196PRG;
	info->Power = M196Power;
}
