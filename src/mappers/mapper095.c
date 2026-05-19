/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2023-2025-2026 negativeExponent
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

/*
Mapper 95 represents NAMCOT-3425, a board used only for the game Dragon Buster (J).

It is to the ordinary Namco 108 family boards (mapper 206) as TKSROM and TLSROM
(mapper 118) is to ordinary MMC3 boards. Instead of having hardwired mirroring
like mapper 206, it has CHR A15 directly controlling CIRAM A10, just as CHR A17
controls CIRAM A10 on TxSROM. Only horizontal mirroring and 1-screen mirroring
are possible because the Namco 108 lacks the C bit of MMC3.
*/

#include "mapinc.h"
#include "n118.h"

static void SyncCHR(void) {
	setchr1(0x0000, n118.reg[0] & 0xFE);
	setchr1(0x0400, n118.reg[0] | 0x01);
	setchr1(0x0800, n118.reg[1] & 0xFE);
	setchr1(0x0C00, n118.reg[1] | 0x01);
	setchr1(0x1000, n118.reg[2]);
	setchr1(0x1400, n118.reg[3]);
	setchr1(0x1800, n118.reg[4]);
	setchr1(0x1C00, n118.reg[5]);
	
	setntamem(NTARAM + (((n118.reg[0] >> 5) & 0x01) << 10), TRUE, 0);
	setntamem(NTARAM + (((n118.reg[0] >> 5) & 0x01) << 10), TRUE, 1);
	setntamem(NTARAM + (((n118.reg[1] >> 5) & 0x01) << 10), TRUE, 2);
	setntamem(NTARAM + (((n118.reg[1] >> 5) & 0x01) << 10), TRUE, 3);
}

void Mapper095_Init(CartInfo *info) {
	N118_Init(info, 0, 0);
	N118_SyncCHR = SyncCHR;
}
