/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 337 - BMC-CTC-12IN1
 * 12-in-1 Game Card multicart
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_337

 * NES 2.0 Mapper 350 - BMC-891227
 * Super 15-in-1 Game Card 
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_350
 */

#include "mapinc.h"

static uint8 isM350;
static uint8 latche;

static SFORMAT StateRegs[] = {
	{ &latche, 1, "REG0" },
	{ &isM350, 1, "M350" },
	{ 0 }
};

static void Sync(void) {
	uint8 mirr, mode, chrp;
	uint8 bank = latche & 0x1F;

	if (isM350) {
		mirr = ((latche >> 7) & 1);
		mode = ((latche >> 5) & 3);
		chrp = (latche & 0x40) != 0;
		if (mode == 3) {
			/* 2nd chip only has 128KB */
			bank &= 7;
			bank |= 0x20;
		}
	} else {
		mirr = ((latche >> 5) & 1);
		mode = ((latche >> 6) & 3);
		chrp = (latche & 0x80) != 0;
	}

	setprg8(0x6000, 1);
	switch (mode) {
	case 0: /* NROM-128 */
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
		break;
	case 1: /* NROM-256 */
		setprg32(0x8000, bank >> 1);
		break;
	case 2:
	case 3: /* UNROM */
		setprg16(0x8000, bank);
		setprg16(0xC000, bank | 7);
		break;
	}

	/* CHR-RAM Protect... kinda */
	SetupCartCHRMapping(0, CHRptr[0], 0x2000, chrp);
	setchr8(0);
	if (isM350 && (latche & 0x40)) {
		/* use default mirroring */
	} else {
		setmirror(mirr ^ 1);
	}
}

static DECLFW(GCOuterBankWrite) {
	latche = (latche & 7) | (V & ~7);
	Sync();
}

static DECLFW(GCInnerBankWrite) {
	latche = (latche & ~7) | (V & 7);
	Sync();
}

static void GCPower(void) {
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xBFFF, GCOuterBankWrite);
	SetWriteHandler(0xC000, 0xFFFF, GCInnerBankWrite);
}

static void GCReset() {
	latche = 0; /* always reset to menu */
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

static void GameCard_Init(CartInfo *info, int m350) {
	isM350 = m350;
	info->Power = GCPower;
	info->Reset = GCReset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}

void BMCCTC12IN1_Init(CartInfo *info) {
	GameCard_Init(info, 0);
}

void BMC891227_Init(CartInfo *info) {
	GameCard_Init(info, 1);
}
